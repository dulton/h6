/* *
 * This file implements event scheduler.
 *
 * Copyright (C) 2014 Shiyong Zhang <shiyong.zhang.cn@gmail.com>
 * See COPYING for more details
 * */
 
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "queue.h"
#include "h6_sched.h"
#include "trace.h"

typedef struct _h6_loop h6_loop;
struct _h6_loop
{
	uint32_t		weight;

	h6_ev_loop		*loop;
	pthread_t		*loop_thread;
};

typedef struct _h6_src_weight h6_src_weight;
struct _h6_src_weight
{
	uint32_t	w;		/* 权重 */

	h6_ev_t		*src;
	h6_loop		*loop;
};

struct _h6_loop_scher
{
    uint32_t        amount;
	h6_loop			*loops;
    
	pthread_mutex_t	*lock;

	uint32_t		count;
	uint32_t		total_weight;
	list_t			*w_list;
};


static void*
h6_loop_thread_fun(h6_loop *loop)
{
	h6_ev_loop_run(loop->loop);
	return NULL;
}


static __inline__ void
h6_loop_init(h6_loop *loop)
{
    int32_t result;
    
	loop->weight = 0;
	loop->loop = h6_ev_loop_new();
	loop->loop_thread = (pthread_t *)malloc(sizeof(pthread_t));
    result = pthread_create(loop->loop_thread, NULL, (void* (*)(void*))&h6_loop_thread_fun, (void *)loop);
    assert(!result);
}


static __inline__ void
h6_loop_add_source(h6_loop *loop, h6_ev_t *src, unsigned weight)
{
    TRACE_ENTER_FUNCTION;
    
	h6_ev_loop_attach(loop->loop, src);
	loop->weight += weight;

    TRACE_EXIT_FUNCTION;
}


static __inline__ void
h6_loop_del_source(h6_loop *loop, h6_ev_t *src, unsigned weight)
{
    TRACE_ENTER_FUNCTION;
    
	h6_ev_loop_remove(loop->loop, src);
	loop->weight -= weight;

    TRACE_EXIT_FUNCTION;
}


h6_scher_t*
h6_sched_new(int32_t loops)
{
	h6_scher_t *sched;
	int32_t size;

	size = loops * sizeof(h6_loop);
	sched = (h6_scher_t *)calloc(1, sizeof(h6_scher_t));
	sched->loops = (h6_loop*)malloc(size);
    sched->amount = loops;
    
    sched->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(sched->lock, NULL);
    
	sched->count = loops;
	sched->total_weight = 0;
	sched->w_list = NULL;

	while (--loops >= 0)
	{
		h6_loop_init(&sched->loops[loops]);
	}

	return sched;
}

static void
h6_sched_free_weight(void *data, void *data_custom)
{
    h6_src_weight *sw = (h6_src_weight *)data;

    free(sw);
}

void
h6_sched_free(h6_scher_t *sched)
{
    uint32_t idx;
    void     *result;

    TRACE_ENTER_FUNCTION;
    
    if (!sched)
        return;
    
    for (idx = 0; idx < sched->amount; idx++)
    {
        h6_ev_loop_quit(sched->loops[idx].loop);
        pthread_join(*(sched->loops[idx].loop_thread), &result);
        free(sched->loops[idx].loop_thread);
        
        h6_ev_loop_unref(sched->loops[idx].loop);
    }
    free(sched->loops);

	list_foreach(
		sched->w_list,
		h6_sched_free_weight,
		NULL
	);
    list_free(sched->w_list);    
    
    if (sched->lock)
    {
        pthread_mutex_destroy(sched->lock);
        free(sched->lock);
    }
    
    free(sched);

    TRACE_EXIT_FUNCTION;
}

static __inline__ h6_loop *
h6_find_best_loop(h6_scher_t *sched)
{
	unsigned w, best_w = sched->total_weight;
	int32_t best_i = 0, idx = 0;

	for (; idx < sched->count; ++idx)
	{
		w = sched->loops[idx].weight;
		if (w < best_w)
		{
			best_w = w;
			best_i = idx;
		}
	}

	return sched->loops + best_i;
}


static __inline__ void
h6_sched_add_weight(h6_scher_t *sched, h6_loop *loop, h6_ev_t *src, 
    uint32_t weight)
{
	h6_src_weight *sw;

	sw = (h6_src_weight *)malloc(sizeof(h6_src_weight));
	sw->w = weight;
	sw->src = src;
	sw->loop = loop;

	sched->total_weight += weight;
	sched->w_list = list_insert_head(sched->w_list, sw);
}

static __inline__ void
h6_sched_del_weight(h6_scher_t *sched, h6_src_weight *sw)
{
    sched->total_weight -= sw->w;
    h6_loop_del_source(sw->loop, sw->src, sw->w);
    free(sw);
}


static int32_t
h6_loop_find_sw(void *a, void *src)
{
	h6_src_weight *sw = (h6_src_weight*)a;

	if (sw->src == (h6_ev_t*)src)
		return 0;
	return 1;
}


static __inline__ int32_t
_h6_sched_remove(h6_scher_t *sched, h6_ev_t *src)
{
	h6_src_weight *sw;
	list_t *list;

    TRACE_ENTER_FUNCTION;
    
	list = list_find_custom(sched->w_list, src, h6_loop_find_sw);
	if (list)
	{   
		sw = (h6_src_weight*)list->data;
		h6_sched_del_weight(sched, sw);
        sched->w_list = list_delete_link(sched->w_list, list); 

        TRACE_EXIT_FUNCTION;
		return 0;
	}
    else
    {
        TRACE_WARNING("Don't find h6_ev_t object(%p) in sched->w_list,\
            function _h6_sched_remove.\r\n", src);
    }

    TRACE_EXIT_FUNCTION;
	return -1;
}

static __inline__ void
_h6_sched_add(h6_scher_t *sched, h6_ev_t *src, uint32_t weight)
{
	h6_loop *loop = h6_find_best_loop(sched);
	h6_loop_add_source(loop, src, weight);
	h6_sched_add_weight(sched, loop, src, weight);
}


int32_t
h6_sched_add(h6_scher_t *sched, h6_ev_t *src, uint32_t weight)
{
	assert(sched != NULL);

	pthread_mutex_lock(sched->lock);
	_h6_sched_add(sched, src, weight);
	pthread_mutex_unlock(sched->lock);

	return 0;
}


int32_t
h6_sched_remove(h6_scher_t *sched, h6_ev_t *src)
{
	int32_t ret;
	assert(sched != NULL);

	pthread_mutex_lock(sched->lock);
	ret = _h6_sched_remove(sched, src);
	pthread_mutex_unlock(sched->lock);

	return ret;
}


//:~ End

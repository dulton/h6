#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "queue.h"
#include "h6_def.h"
#include "h6_ev.h"

#define FLAGS_EVENT_IN_LOOP 0x01
    
#if defined (__GNUC__) && __GNUC__ > 2 /* since 2.9 */
	# define H6_LIKELY(expr) (__builtin_expect(!!(expr), 1))
	# define H6_UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
#else
	# define H6_LIKELY(expr) (expr)
	# define H6_UNLIKELY(expr) (expr)
#endif

#define BUG_ON(x) \
do {\
	if (x) \
	{\
		fprintf(stderr, "BUG_ON(%s) at file:%s line:%d function:%s!\n", #x, __FILE__, __LINE__, __FUNCTION__); \
		char *_______________________p = 0; *_______________________p = 0; \
	}\
} while (0)

#ifndef REF_DEBUG_TEST
#define MAX_REF_COUNT_VALUE (10000)	/* Maybe enough */
#define REF_DEBUG_TEST(pobj)	\
    do {\
        assert((pobj) && atomic_get(&(pobj)->ref_count) > 0); \
        assert(atomic_get(&(pobj)->ref_count) < MAX_REF_COUNT_VALUE); \
    } while (0)
#endif



typedef struct _h6_ev_loop_ops h6_ev_loop_ops;
typedef void (*loop_ops_func)(h6_ev_loop *loop, h6_ev_loop_ops *ops);

typedef enum _loop_ops_type ops_type_t;
enum _loop_ops_type
{
    unknown_ops_type = 0,
    io_ops_type,
    timer_ops_type
};

struct _h6_ev_loop_ops
{
	h6_ev_t             *event;
	ops_type_t          ops_type; // enum _loop_ops_typs
	loop_ops_func       func;
};

struct wakeup_block		/* for loop waking up */
{
	int             request_pending;
	struct ev_io    waker;
	int             wakeup_pipe[2];
};

struct _h6_ev_loop_t
{
    atomic_t        ref_count;

	struct ev_loop  *ev_loop;
	int32_t         loop_running;
	int32_t         loop_quited;
    
    struct wakeup_block wakeup;
    
    // following variables is for async operation of loop, such as 
    // add/remove/modify event I use a queue to store all operation,
    // then wake up loop to execute opeation
    pthread_mutex_t *lock;
    queue_t         *operations;
    list_t          *ev_list;
};


static void h6_ev_cb_io(EV_P_ struct ev_io *w, int revents);
static void h6_ev_cb_timeout(EV_P_ struct ev_timer *w, int revents);
static void h6_ev_loop_force_woken_up(h6_ev_loop *loop);
static void h6_ev_loop_sync_remove(h6_ev_loop *loop, h6_ev_t *event);

static void
h6_ev_loop_wakeup_cb(EV_P_ struct ev_io *w, int revents)
{
	h6_ev_loop *_loop;

	_loop = container_of(w, h6_ev_loop, wakeup.waker);

	if (revents & EV_READ)
	{
		h6_ev_loop_force_woken_up(_loop);
		revents &= ~EV_READ;
	}

	BUG_ON(revents);
}

static int32_t
h6_ev_wakeup_init(struct wakeup_block *wb, h6_ev_loop *loop)
{
	if (pipe(wb->wakeup_pipe) < 0)
		return -1;

	ev_io_init(&wb->waker, h6_ev_loop_wakeup_cb, wb->wakeup_pipe[0], EV_READ);
	ev_io_start(loop->ev_loop, &wb->waker);

    return 0;
}

static __inline__ void
h6_ev_block_wakup(struct wakeup_block *wb)
{
	assert(wb != NULL);

	if (wb->request_pending)
		return;
	wb->request_pending = 1;
	write(wb->wakeup_pipe[1], "x", 1);
}

static void
h6_ev_loop_wakeup(h6_ev_loop *loop)
{
	h6_ev_block_wakup(&loop->wakeup);
}

static __inline__ void
h6_ev_block_woken_up(struct wakeup_block *wb)
{
	char x;
	assert(wb != NULL);

	wb->request_pending = 0;
	read(wb->wakeup_pipe[0], &x, 1);
}

//-------------------------------------------------------------------

static __inline__ void
h6_ev_loop_free(h6_ev_loop *loop)
{
    // to do ...check
    if (loop == NULL)
        return;
    
    if (loop->ev_loop)
        ev_loop_destroy(loop->ev_loop);
    
    if (loop->lock)
    {
        pthread_mutex_destroy(loop->lock);
        free(loop->lock);
    }
    
    if (loop->operations)
        queue_free(loop->operations);   
    
    if (loop->ev_list)
        list_free(loop->ev_list);
        
    free(loop);
}

static __inline__ int32_t
h6_ev_loop_init(h6_ev_loop *loop)
{
    atomic_set(&loop->ref_count, 1);

	loop->ev_loop = h6_ev_loop_new();
	if (loop->ev_loop == NULL)
        return -1;

    if (h6_ev_wakeup_init(&loop->wakeup, loop) != 0)
    {
        ev_loop_destroy(loop->ev_loop);
        return -1;
    }
	loop->loop_running = 0;
	loop->loop_quited = 0;
            
    loop->lock = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    if (loop->lock == NULL)
    {
        h6_ev_loop_free(loop);
        return -1;
    }
    pthread_mutex_init(loop->lock, NULL);
        
    loop->operations = queue_new();
    if (loop->operations == NULL)
    {
        h6_ev_loop_free(loop);
        return -1;    
    }
    
    loop->ev_list = NULL;
    
	return 0;
}

h6_ev_loop *
h6_ev_loop_new(void)
{
	h6_ev_loop *loop;

	loop = (h6_ev_loop *)calloc(1, sizeof(h6_ev_loop));
	if (h6_ev_loop_init(loop))
	{
		free(loop);
		return NULL;
	}
    
	return loop;
}

h6_ev_loop *
h6_ev_loop_ref(h6_ev_loop *loop)
{
    REF_DEBUG_TEST(loop);
    atomic_inc(&loop->ref_count);
    
	return loop;
}



void
h6_ev_loop_unref(h6_ev_loop *loop)
{
	REF_DEBUG_TEST(loop);
	if (atomic_dec_and_test_zero(&loop->ref_count))
	{
		h6_ev_loop_free(loop);
	}

}



void
h6_ev_loop_run(h6_ev_loop *loop)
{
	int32_t to_run;
	assert(loop != NULL);

	if (loop->loop_quited)
		to_run = 0;
	else
	{
		to_run = !loop->loop_running;
		loop->loop_running = 1;
	}

	if (to_run)
		ev_run(loop->ev_loop, 0);
}

static void
h6_ev_realse_one(void *data_orig, void *data_custom)
{
	h6_ev_t *event;
	assert(data_orig != NULL);

	event = (h6_ev_t*)data_orig;
	BUG_ON(!event->opt->loop);
	h6_ev_loop_sync_remove(event->opt->loop, event);
}

static void
h6_ev_loop_ops_quit(h6_ev_loop *loop, h6_ev_loop_ops *null)
{
	list_foreach(
		loop->ev_list,
		h6_ev_realse_one,
		NULL
	);

	list_free(loop->ev_list);
	loop->ev_list = NULL;

	ev_break(loop->ev_loop, EVBREAK_ALL);
}

void h6_ev_loop_quit(h6_ev_loop *loop)
{
	h6_ev_loop_ops *ops;
	assert(loop != NULL);

	pthread_mutex_lock(loop->lock);

	if (!loop->loop_quited)
	{
		ops = calloc(1, sizeof(h6_ev_loop_ops));
		ops->func = h6_ev_loop_ops_quit;

		loop->loop_quited = 1;        
        queue_push_tail(loop->operations, ops);
        
		h6_ev_loop_wakeup(loop);
	}

	pthread_mutex_unlock(loop->lock);
}

static __inline__ void
h6_ev_loop_sync_add(h6_ev_loop *loop, h6_ev_t *event)
{
	struct ev_loop *_loop;
	ev_tstamp to;
	assert(loop != NULL && event != NULL);

	_loop = loop->ev_loop;
	BUG_ON(!_loop);

	if (event->ev_fd >= 0)
	{
		ev_io_init(&event->io, h6_ev_cb_io, event->ev_fd, event->events);

		if (event->events)
			ev_io_start(_loop, &event->io);
	}

	if (event->timeout >= 0)
	{
		to = ((ev_tstamp)event->timeout)/1000;
		ev_timer_init(&event->timer, h6_ev_cb_timeout, to, to);
		ev_timer_start(_loop, &event->timer);
	}

	event->flags |= FLAGS_EVENT_IN_LOOP;
}

static void
h6_ev_loop_ops_add(h6_ev_loop *loop, h6_ev_loop_ops *ops)
{
	h6_ev_t *event;

	event = ops->event;
    loop->ev_list = list_insert_head(loop->ev_list, event);
	h6_ev_loop_sync_add(loop, event);
}

static __inline__ h6_bool_t
h6_ev_loop_add(h6_ev_loop *loop, h6_ev_t *event)
{
	h6_bool_t ret = H6_FALSE;
    h6_ev_loop_ops *ops;

    assert(event->opt->loop == NULL);
    
    pthread_mutex_lock(loop->lock);
    
    if (!loop->loop_quited)
    {
        event->opt->loop = loop;
        
        ops = (h6_ev_loop_ops *)calloc(1, sizeof(h6_ev_loop_ops));
        ops->event = event;
        ops->func = h6_ev_loop_ops_add;
        
        queue_push_tail(loop->operations, ops);
        h6_ev_loop_wakeup(loop);
        
        ret = H6_TRUE;
    }
    
    pthread_mutex_unlock(loop->lock);
    
    return ret;
}

h6_bool_t
h6_ev_loop_attach(h6_ev_loop *loop, h6_ev_t *event)
{
	assert(loop != NULL && event != NULL);

	return h6_ev_loop_add(loop, event);
}

static void
h6_ev_loop_sync_remove(h6_ev_loop *loop, h6_ev_t *event)
{
	struct ev_loop *_loop;
	assert(loop != NULL && event != NULL);

	_loop = loop->ev_loop;
	BUG_ON(!_loop);

	if (event->ev_fd >= 0)
		ev_io_stop(_loop, &event->io);

	if (event->timeout >= 0)
		ev_timer_stop(_loop, &event->timer);

	event->flags &= ~FLAGS_EVENT_IN_LOOP;
}

static void
h6_ev_loop_ops_remove(h6_ev_loop *loop, h6_ev_loop_ops *ops)
{
    list_t *list;
    
    assert(loop != NULL && ops != NULL);
    list = list_find(loop->ev_list, ops->event);
    if (list)
    {
        loop->ev_list = list_remove_link(loop->ev_list, list);
        h6_ev_loop_sync_remove(loop, ops->event);
    }
}

void
h6_ev_loop_remove(h6_ev_loop *loop, h6_ev_t *event)
{
	h6_ev_loop_ops *ops;
	assert(event != NULL && loop != NULL);

    BUG_ON(loop != event->opt->loop);
        
	pthread_mutex_lock(loop->lock);

	if (!loop->loop_quited)
	{
		ops = (h6_ev_loop_ops *)calloc(1, sizeof(h6_ev_loop_ops));
		ops->event = event;
		ops->func = h6_ev_loop_ops_remove;

		queue_push_tail(loop->operations, ops);
		h6_ev_loop_wakeup(event->opt->loop);
	}

	pthread_mutex_unlock(event->opt->loop->lock);
}

static __inline__ void
h6_ev_loop_sync_io_modified(h6_ev_loop *loop, h6_ev_t *event, ops_type_t type)
{
	struct ev_loop *_loop;

	_loop = loop->ev_loop;
	BUG_ON(!_loop);

	if (!(event->flags  & FLAGS_EVENT_IN_LOOP))
		return;

	if (type == io_ops_type)
	{
		if (event->ev_fd >= 0)
		{
			ev_io_stop(_loop, &event->io);

			if (event->events)
			{
                ev_io_init(&event->io, h6_ev_cb_io, event->ev_fd, event->events);
                ev_io_start(_loop, &event->io);
			}
		}
	}
}

static void
h6_ev_loop_ops_modify(h6_ev_loop *loop, h6_ev_loop_ops *ops)
{
	h6_ev_loop_sync_io_modified(loop, ops->event, ops->ops_type);
}

static __inline__ void
h6_ev_loop_modify_event(h6_ev_loop *loop, h6_ev_t *event, ops_type_t ops_type)
{
	h6_ev_loop_ops *ops;

	pthread_mutex_lock(loop->lock);
    
    if (!loop->loop_quited)
	{
		ops = (h6_ev_loop_ops *)calloc(1, sizeof(h6_ev_loop_ops));
		ops->event = event;
		ops->ops_type = ops_type;
		ops->func = h6_ev_loop_ops_modify;

		queue_push_tail(loop->operations, ops);
		h6_ev_loop_wakeup(loop);
	}

	pthread_mutex_unlock(loop->lock);
}




static __inline__ void
h6_ev_loop_exec_operations(h6_ev_loop *loop)
{
	h6_ev_loop_ops *ops;

	while ((ops = queue_pop_head(loop->operations)))
	{
		(*ops->func)(loop, ops);
		pthread_mutex_unlock(loop->lock);

		free(ops);
		pthread_mutex_lock(loop->lock);
	}
}

static void
h6_ev_loop_force_woken_up(h6_ev_loop *loop)
{
	assert(loop != NULL);

	pthread_mutex_lock(loop->lock);

	h6_ev_block_woken_up(&loop->wakeup);
	h6_ev_loop_exec_operations(loop);

	pthread_mutex_unlock(loop->lock);
}

/**
 * call function to process event
 * @return 1 - succeed, normally 1(succeed) means that loop should continue
 *             to watch this event, and timeout watcher continue to work
 *         0 - false, timeout watcher will stop
 */
static __inline__ int32_t
h6_ev_call(h6_ev_t *ev, int revents)
{
	assert(ev != NULL);
	return (*ev->opt->dispath)(ev, revents, ev->user_data);
}
   
static __inline__ void
h6_ev_reset_timer(h6_ev_t *ev)
{
	if (ev->timeout > 0)
	{
		ev_timer_again(ev->opt->loop->ev_loop, &ev->timer);
	}
}

static void
h6_ev_cb_io(EV_P_ struct ev_io *w, int revents)
{
	h6_ev_t *ev = container_of(w, h6_ev_t, io);

	if (h6_ev_call(ev, revents))
        h6_ev_reset_timer(ev);
}

static void
h6_ev_cb_timeout(EV_P_ struct ev_timer *w, int revents)
{
	h6_ev_t *ev = container_of(w, h6_ev_t, timer);

	h6_ev_call(ev, revents);
}

h6_ev_t *
h6_ev_new(size_t size, int fd, int events)
{
	h6_ev_t *ev;

	if (size < sizeof(h6_ev_t))
		return NULL;

	ev = (h6_ev_t *)malloc(size);
	memset(ev, 0, size);

    atomic_set(&ev->ref_count, 1);
	ev->ev_size = size;

	events &= ( EV_READ | EV_WRITE );
    
	ev_io_init(&ev->io, h6_ev_cb_io, fd, events);
	ev_timer_init(&ev->timer, h6_ev_cb_timeout, .0, .0);

	ev->ev_fd = fd;
	ev->timeout = -1;
	ev->events = events;

	return ev;
}

h6_ev_t *h6_ev_ref(h6_ev_t *event)
{
	REF_DEBUG_TEST(event);
	atomic_inc(&event->ref_count);
	return event;
}


void h6_ev_unref(h6_ev_t *event)
{
	REF_DEBUG_TEST(event);
	if (atomic_dec_and_test_zero(&event->ref_count))
	{
		if (event->opt->destroy)
			(*event->opt->destroy)(event);
		free(event);
	}
}


void
h6_ev_set_callback(h6_ev_t *event, h6_ev_dispath dispath,
	void *user_data, h6_ev_ondestroy notify)
{
	assert(event != NULL);

	event->user_data = user_data;
	event->opt->dispath = dispath;
	event->opt->destroy = notify;
}

void
h6_ev_set_timeout(h6_ev_t *event, int timeout)
{
	assert(event != NULL);

	if (!event->opt->loop)	/* race against h6_ev_loop_attach() */
	{
		event->timeout = timeout;
		return;
	}
}

void
h6_ev_remove_events(h6_ev_t *event, int events)
{
	assert(event != NULL);
    
    if (!events)
        return;

	if (event->events & events)
	{
		event->events &= ~events;
		if (event->opt->loop)
			h6_ev_loop_modify_event(event->opt->loop, event, io_ops_type);
	}    
}

void
h6_ev_add_events(h6_ev_t *event, int events)
{
	assert(event != NULL);

	if (!events)
		return;

	if ((event->events & events) != events)
	{
		event->events |= events;
		if (event->opt->loop)
			h6_ev_loop_modify_event(event->opt->loop, event, io_ops_type);
	}
}

void
h6_ev_add_events_sync(h6_ev_t *event, int events)
{
	assert(event != NULL);

	if ((event->events & events) != events)
	{
		event->events |= events;
		h6_ev_loop_sync_io_modified(event->opt->loop, event, io_ops_type);
	}
}

void
h6_ev_mod_timer_sync(h6_ev_t *event, ev_tstamp after)
{
	int milli_secs;
	assert(event != NULL);

	if (H6_UNLIKELY(after <= 0))
	{
		after = .000001;
		milli_secs = 1;
	}
	else
	{
		milli_secs = after * 1000;
		if (milli_secs <= 0)
			milli_secs = 1;
	}

	event->timeout = milli_secs;
	ev_timer_set(&event->timer, .0, after);
	ev_timer_again(event->opt->loop->ev_loop, &event->timer);
}

void
h6_ev_remove_events_sync(h6_ev_t *event, int events)
{
	assert(event != NULL);

	if (event->events & events)
	{
		event->events &= ~events;
		h6_ev_loop_sync_io_modified(event->opt->loop, event, io_ops_type);
	}
}

ev_tstamp
h6_ev_time_now_sync(h6_ev_t *event)
{
	assert(event != NULL);

	return ev_now(event->opt->loop->ev_loop);
}

// ------------------------------------------------------------------


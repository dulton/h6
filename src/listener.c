#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "listener.h"
#include "trace.h"

static __inline__ void
listener_init_this(listener_t *l)
{
    INIT_LIST_HEAD(&l->list_node);
    l->server = NULL;
}

static __inline__ void
listener_fin_this(listener_t *l)
{
    assert(list_empty(&l->list_node));
}

static __inline__ void
on_listener_fin(obj_t *p)
{
	listener_t *l = (listener_t*)p;

	if (l->ops && l->ops->fin)
	{
		(*l->ops->fin)(l);
	}

	listener_fin_this(l);
}

listener_t *
listener_alloc(uint32_t size, listener_ops *ops, void *u)
{
	listener_t *l;
	int32_t err;

    TRACE_ENTER_FUNCTION;
    
	if (size < sizeof(*l))
	{
        TRACE_EXIT_FUNCTION;
		return NULL;
	}

	l = (listener_t*)obj_new(size, on_listener_fin, __FUNCTION__);
    if (l)
    {
        listener_init_this(l);

    	if (ops && ops->init)
    	{
    		err = (*ops->init)(l, u);
    		if (err)
    		{
    			obj_unref((obj_t *)l);

                TRACE_EXIT_FUNCTION;
    			return NULL;
    		}
    	}

    	l->ops = ops;
    }

    TRACE_EXIT_FUNCTION;
	return l;
}


int32_t
listener_bind(listener_t *l, uint16_t port)
{
	int32_t err = -EINVAL;

	if (l->ops && l->ops->set_port)
	{
		err = (*l->ops->set_port)(l, port);
	}

	return err;
}

static __inline__ void
listener_self_kill(listener_t *l)
{
    if (l->lset)
        lsn_set_del(l->lset, l);
}

void
listener_kill(listener_t *l)
{
    TRACE_ENTER_FUNCTION;
    
	if (l->ops && l->ops->kill)
	{
	    (*l->ops->kill)(l);
	}

    listener_self_kill(l);

    TRACE_EXIT_FUNCTION;
}

static __inline__ void
listener_unref(listener_t *l)
{
    if (l)
        obj_unref(l);
}

void
listener_kill_unref(listener_t *l)
{
    TRACE_ENTER_FUNCTION;
    
    listener_kill(l);
    listener_unref(l);

    TRACE_EXIT_FUNCTION;
}

client_t *
listener_generate(listener_t *l, void *parm)
{
	client_t *c = NULL;

	if (l->ops && l->ops->new_cli)
	{
		c = (*l->ops->new_cli)(l, parm);
	}
    
	return c;
}


void listener_set_owner(listener_t *l, void *p)
{
	l->server = p;
}


void *listener_get_owner(listener_t *l)
{
	return l->server;
}


static __inline__ void
lsn_set_init_this(lsn_set_t *lset)
{
    TRACE_ENTER_FUNCTION;
    
    INIT_LIST_HEAD(&lset->lsn_list);

    lset->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(lset->lock, NULL);

    TRACE_EXIT_FUNCTION;
}


static __inline__ void
lsn_set_fin_this(lsn_set_t *lset)
{
    TRACE_ENTER_FUNCTION;
    
    // ensure self had been removed from list
    assert(list_empty(&lset->lsn_list));

    pthread_mutex_destroy(lset->lock);
    free(lset->lock);

    TRACE_EXIT_FUNCTION;
}


static __inline__ void
on_lsn_set_fin(obj_t *p)
{
	lsn_set_t *lset = (lsn_set_t*)p;

    TRACE_ENTER_FUNCTION;
    
	if (lset->ops && lset->ops->fin)
	{
		(*lset->ops->fin)(lset);
	}

	lsn_set_fin_this(lset);

    TRACE_EXIT_FUNCTION;
}


static lsn_set_t *
lsn_set_alloc(uint32_t size, lsn_set_ops *ops, void *u, const char *name)
{
	int32_t err;
	lsn_set_t *lset;

    TRACE_ENTER_FUNCTION;
    
	if (size < sizeof(*lset))
	{
        TRACE_EXIT_FUNCTION;
		return NULL;
	}
    
	lset = (lsn_set_t *)obj_new(size, on_lsn_set_fin, name);
	if (lset)
	{
		lsn_set_init_this(lset);
		lset->ops = ops;

		if (ops && ops->init)
		{
			err = (*ops->init)(lset, u);
			if (err)
			{
				lset->ops = NULL;
				obj_unref((obj_t *)lset);	//@{fin_this()}

                TRACE_EXIT_FUNCTION;
				return NULL;
			}
		}
	}

    TRACE_EXIT_FUNCTION;
	return lset;    
}

lsn_set_t *
lsn_set_new()
{
    return lsn_set_alloc(sizeof(lsn_set_t), NULL, NULL, __FUNCTION__);
}

lsn_set_t *
lsn_set_ref(lsn_set_t *set)
{
    if (set)
        obj_ref(set);

    return set;
}

void
lsn_set_unref(lsn_set_t *set)
{
    if (set)
        obj_unref(set);    
}

static __inline__ void
lsn_set_self_kill(lsn_set_t *set)
{
    struct list_head *node, *n;
    listener_t *l;

    TRACE_ENTER_FUNCTION;
    
    pthread_mutex_lock(set->lock);
    
    list_for_each_safe(node, n, &set->lsn_list)
    {
        l = list_entry(node, listener_t, list_node);
        list_del(&l->list_node);
        
        listener_kill_unref(l);
    }
    assert(list_empty(&set->lsn_list));

    pthread_mutex_unlock(set->lock);

    TRACE_EXIT_FUNCTION;
}

static __inline__ void
lsn_set_kill(lsn_set_t *set)
{   
    TRACE_ENTER_FUNCTION;
    
	if (set->ops && set->ops->kill)
	{
		(*set->ops->kill)(set);
	}

    lsn_set_self_kill(set);

    TRACE_EXIT_FUNCTION;
}

void
lsn_set_kill_unref(lsn_set_t *set)
{
    TRACE_ENTER_FUNCTION;
    
    lsn_set_kill(set);
    lsn_set_unref(set);

    TRACE_EXIT_FUNCTION;    
}

static int32_t
__listen_set_add(lsn_set_t *set, listener_t *l)
{
	int32_t err = -EINVAL;

	if (list_empty(&l->list_node))
	{
		err = 0;
		list_add(&l->list_node, &set->lsn_list);

        obj_ref((obj_t *)l);
	}

	return err;    
}

int32_t
lsn_set_add(lsn_set_t *set, listener_t *l)
{
	int32_t err;

	pthread_mutex_lock(set->lock);
	err = __listen_set_add(set, l);
	pthread_mutex_unlock(set->lock);

	return err;    
}

static __inline__ int32_t
__listen_set_del(lsn_set_t *set, listener_t *l)
{
	int32_t err = -EINVAL;

	if (!list_empty(&l->list_node))
	{
		err = 0;
		list_del_init(&l->list_node);

        obj_unref((obj_t *)l);
	}

	return err;
}

void
lsn_set_del(lsn_set_t *set, listener_t *l)
{
	pthread_mutex_lock(set->lock);
	__listen_set_del(set, l);
	pthread_mutex_unlock(set->lock);    
}


//:~ End

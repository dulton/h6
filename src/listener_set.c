#include <assert.h>
#include <errno.h>
#include "listener_set.c"

struct __listener_set_ops
{
	int32_t (*init)(lsn_set_t *lset, void *u);
	void	(*fin)(lsn_set_t *lset);
	void	(*kill)(lsn_set_t *lset);    
}

static __inline__ void
init_this(lsn_set_t *lset)
{
    INIT_LIST_HEAD(&lset->lsn_list);
    lset->lock = NULL;
}


static __inline__ void
fin_this(lsn_set_t *lset)
{
    // ensure self had been removed from list
	assert(list_empty(&lset->list_node));
}


static __inline__ void
on_lsn_set_fin(obj_t *p)
{
	lsn_set_t *lset = (lsn_set_t*)p;

	if (lset->ops && lset->ops->fin)
	{
		(*lset->ops->fin)(lset);
	}

	fin_this(lset);
}


static lsn_set_t *
lsn_set_alloc(uint32_t size, lsn_set_ops *ops, void *u, const char *name)
{
	int32_t err;
	lsn_set_t *lset;

	if (size < sizeof(*lset))
		return NULL;
    
	lset = (lsn_set_t *)obj_new(size, on_lsn_set_fin, name);
	if (lset)
	{
		init_this(lset);
		lset->ops = ops;

		if (ops && ops->init)
		{
			err = (*ops->init)(lset, u);
			if (err)
			{
				lset->ops = NULL;
				obj_unref((obj_t *)lset);	//@{fin_this()}
				return NULL;
			}
		}
	}

	return lset;    
}

static int32_t
listener_set_init(lsn_set_t *lset, void *u)
{
    assert(lset);
    
    lset->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(lset->lock);
}

void
listener_set_destroy(lsn_set_t *lset)
{
    assert(lset);

    pthread_mutex_destroy(lset->lock);
    free(lset->lock);    
}


static struct __listener_set_ops lsn_ops
{
    .init = listener_set_init;
    .fin  = listener_set_destroy;
    .kill = NULL;
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
    // nothing to do at this moment
}

static __inline__ void
lsn_set_kill(lsn_set_t *set)
{   
	if (set->ops && set->ops->kill)
	{
		(*set->ops->kill)(set);
	}

    lsn_set_self_kill(set);
}

void
lsn_set_kill_unref(lsn_set_t *set)
{
    lsn_set_kill(set);
    lsn_set_unref(set);
}

static int32_t
__listen_set_add(lsn_set_t *set, listener_t *l)
{
	int32_t err = -EINVAL;

	if (list_empty(&l->list_node))
	{
		err = 0;
		list_add(&l->list_node, &set->c_list);
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




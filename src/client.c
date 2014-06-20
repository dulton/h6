#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "client.h"
#include "trace.h"

static atomic_t client_objs = ATOMIC_INIT;

static __inline__ void
client_init_this(client_t *c)
{
    INIT_LIST_HEAD(&c->list_node);

    c->cs = NULL;
	c->sched = NULL;
	c->ops = NULL;    

    atomic_inc(&client_objs);
}

static __inline__ void
client_fin_this(client_t *c)
{
    // ensure self had been removed from list
	assert(list_empty(&c->list_node));
    
	atomic_dec(&client_objs);

	TRACE_TRACE(
		"Client objs count:%d.",
		atomic_get(&client_objs)
	);	
}

static __inline__ void
on_client_fin(obj_t *p)
{
	client_t *c = (client_t*)p;

	if (c->ops && c->ops->fin)
	{
		(*c->ops->fin)(c);
	}

	client_fin_this(c);
}

client_t *
client_alloc(uint32_t size, client_ops *ops, void *u, const char *name)
{
	int32_t err;
	client_t *c;

	if (size < sizeof(*c))
		return NULL;
    
	c = (client_t*)obj_new(size, on_client_fin, name);
	if (c)
	{
		client_init_this(c);
		c->ops = ops;

		if (ops && ops->init)
		{
			err = (*ops->init)(c, u);
			if (err)
			{
				c->ops = NULL;
				obj_unref((obj_t *)c);	//@{fin_this()}
				return NULL;
			}
		}
	}

	return c;
}

client_t *
client_ref(client_t *c)
{
	if (c)
	{
		obj_ref(c);
	}

	return c;
}

void
client_unref(client_t *c)
{
	if (c)
		obj_unref(c);
}


static __inline__ void
client_self_kill(client_t *c)
{
    if (c->cs)
        client_set_del(c->cs, c);
}

static __inline__ void
client_kill(client_t *c)
{   
	if (c->ops && c->ops->kill)
	{
		(*c->ops->kill)(c);
	}

    client_self_kill(c);
}

void
client_kill_unref(client_t *c)
{
	client_kill(c);
	client_unref(c);    
}


int32_t
client_attach(client_t *c, void *sched)
{
	int32_t err = -EINVAL;

	c->sched = sched;
	if (c->ops && c->ops->attach)
	{
		err = (*c->ops->attach)(c, sched);
	}

	return err;
}


//===========================================================================

void init_client_set_this(client_set_t *cs)
{
	cs->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(cs->lock, NULL);
    
	INIT_LIST_HEAD(&cs->c_list);
}

static __inline__ void
client_set_fin_this(client_set_t *set)
{
    // ensure self had been removed from list
	assert(list_empty(&set->c_list));

    pthread_mutex_destroy(set->lock);
    free(set->lock);
}


static __inline__ void
on_client_set_fin(obj_t *p)
{
	client_set_t *set = (client_set_t*)p;

	client_set_fin_this(set);
}


static client_set_t *
client_set_alloc(uint32_t size, void *u, const char *name)
{
	client_set_t *cli_set;

	if (size < sizeof(*cli_set))
		return NULL;
    
	cli_set = (client_set_t *)obj_new(size, on_client_set_fin, name);
	if (cli_set)
		init_client_set_this(cli_set);

	return cli_set;     
}

client_set_t* client_set_new()
{
    return (client_set_t *)client_set_alloc(sizeof(client_set_t), NULL, __FUNCTION__);
}

client_set_t *
client_set_ref(client_set_t *set)
{
    if (set)
        obj_ref(set);

    return set;
}

void
client_set_unref(client_set_t *set)
{
    if (set)
        obj_unref(set);
}

static __inline__ void
__client_set_self_kill(client_set_t *set)
{
    struct list_head *node, *n;
    client_t *c;
    
    list_for_each_safe(node, n, &set->c_list)
    {
        c = list_entry(node, client_t, list_node);
        list_del(&c->list_node);
        
        client_kill_unref(c);
    }

    assert(list_empty(&set->c_list));    
}

static __inline__ void
client_set_self_kill(client_set_t *set)
{
	pthread_mutex_lock(set->lock);    
    __client_set_self_kill(set);
	pthread_mutex_unlock(set->lock);    
}

static __inline__ void
client_set_kill(client_set_t *set)
{   
    client_set_self_kill(set);
}

void
client_set_kill_unref(client_set_t *cs)
{
    client_set_kill(cs);
    client_set_unref(cs);
}

static __inline__ int32_t
__client_set_add(client_set_t *cs, client_t *c)
{
	int32_t err = -EINVAL;

	if (list_empty(&c->list_node))
	{
		err = 0;
        c->cs = cs;        
		list_add(&c->list_node, &cs->c_list);
	}

	return err;
}


int32_t client_set_add(client_set_t *cs, client_t *c)
{
	int32_t err;

	pthread_mutex_lock(cs->lock);
	err = __client_set_add(cs, c);
	pthread_mutex_unlock(cs->lock);

	return err;
}


static __inline__ int32_t
__client_set_del(client_set_t *cs, client_t *c)
{
	int32_t err = -EINVAL;

	if (!list_empty(&c->list_node))
	{
		err = 0;
		list_del_init(&c->list_node);
	}

	return err;
}


void client_set_del(client_set_t *cs, client_t *c)
{
	pthread_mutex_lock(cs->lock);
	__client_set_del(cs, c);
	pthread_mutex_unlock(cs->lock);
}


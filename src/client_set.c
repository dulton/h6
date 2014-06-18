#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include "client_set.h"

void init_this(client_set_t *cs)
{
	cs->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(cs->lock, NULL);
    
	INIT_LIST_HEAD(&cs->c_list);
}

static __inline__ void
fin_this(client_set_t *set)
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

	if (set->ops && set->ops->fin)
	{
		(*set->ops->fin)(set);
	}

	fin_this(set);
}


static client_set_t *
client_set_alloc(uint32_t size, client_set_ops *ops, void *u, const char *name)
{
	int32_t err;
	client_set_t *cli_set;

	if (size < sizeof(*cli_set))
		return NULL;
    
	cli_set = (client_set_t *)obj_new(size, on_client_set_fin, name);
	if (cli_set)
	{
		init_this(cli_set);
		cli_set->ops = ops;

		if (ops && ops->init)
		{
			err = (*ops->init)(cli_set, u);
			if (err)
			{
				cli_set->ops = NULL;
				obj_unref((obj_t *)cli_set);	//@{fin_this()}
				return NULL;
			}
		}
	}

	return cli_set;     
}

client_set_t* client_set_new()
{
    return (client_set_t *)client_set_alloc(sizeof(client_set_alloc), NULL, NULL, __FUNCTION__);
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
client_set_self_kill(client_set_t *set)
{
    // nothing to do at this moment
}

static __inline__ void
client_set_kill(client_set_t *set)
{   
	if (set->ops && set->ops->kill)
	{
		(*set->ops->kill)(set);
	}

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



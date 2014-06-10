#include <errno.h>
#include "client.h"
#include "trace.h"

static atomic_t client_objs = ATOMIC_INIT;

static __inline__ void
init_this(client_t *c)
{
    INIT_LIST_HEAD(&c->list_node);
    
	c->sched = NULL;
	c->ops = NULL;

    atomic_inc(&client_objs);
}

static __inline__ void
fin_this(client_t *c)
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

	fin_this(c);
}

client_t *
client_alloc(uint32_t size, client_ops *ops, void *u, uint8_t *name)
{
	int32_t err;
	client_t *c;

	if (size < sizeof(*c))
		return NULL;
    
	c = (client_t*)obj_new(size, on_client_fin, name);
	if (c)
	{
		init_this(c);
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
	{
		obj_unref(c);
	}
}


static __inline__ void
client_self_kill(client_t *c)
{
    // nothing to do at this moment
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

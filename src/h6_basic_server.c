#include "h6_basic_server.h"
#include "trace.h"

static __inline__ void
init_this(h6_svr_t *s)
{    
	s->sched = NULL;
	s->ops = NULL;
}

static __inline__ void
fin_this(h6_svr_t *s)
{
    // ensure self had been removed from list
    TRACE_DEBUG("server objs(%p) is destroy...\r\n", s);
}

static __inline__ void
on_svr_fin(obj_t *p)
{
	h6_svr_t *s = (h6_svr_t*)p;

	if (s->ops && s->ops->fin)
	{
	    (*s->ops->fin)(s);
	}

	fin_this(s);
}

h6_svr_t *
h6_server_alloc(uint32_t size, h6_svr_ops *ops, void *u, const char *name)
{
	int32_t err;
	h6_svr_t *s;

	if (size < sizeof(h6_svr_t))
		return NULL;
    
	s = (h6_svr_t *)obj_new(size, on_svr_fin, name);
	if (s)
	{
		init_this(s);
		s->ops = ops;

		if (ops && ops->init)
		{
			err = (*ops->init)(s, u);
			if (err)
			{
				s->ops = NULL;
				obj_unref((obj_t *)s);	//@{fin_this()}
				return NULL;
			}
		}
	}

	return s;    
}

h6_svr_t *
h6_server_ref(h6_svr_t *s)
{
    if (s)
        obj_ref(s);

    return s;
}

void
h6_server_unref(h6_svr_t *s)
{
    if (s)
        obj_unref(s);    
}

static __inline__ void
server_self_kill(h6_svr_t *s)
{
    // nothing to do at this moment
}

static __inline__ void
server_kill(h6_svr_t *s)
{   
	if (s->ops && s->ops->kill)
	{
	    (*s->ops->kill)(s);
	}

    server_self_kill(s);
}

void
h6_server_kill_unref(h6_svr_t *s)
{
    server_kill(s);
    h6_server_unref(s);
}

int32_t
h6_server_attach(h6_svr_t *s, void *sched)
{
	int32_t err = -EINVAL;

	s->sched = sched;
	if (s->ops && s->ops->attach)
	{
		err = (*s->ops->attach)(s, sched);
	}

	return err;    
}

int32_t
h6_server_add_client(h6_svr_t *s, void *u)
{
    int32_t err = -EINVAL;

    if (s->ops && s->ops->add_client)
    {
        err = (*s->ops->add_client)(s, u);
    }

    return err;
}




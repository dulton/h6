#include <assert.h>
#include "h6_basic_server.h"
#include "trace.h"

static __inline__ void
h6_server_init_this(h6_svr_t *s)
{   
	s->sched = NULL;
	s->ops = NULL;

    TRACE_DEBUG("h6 server objs(%p) created...\r\n", s);
}

static __inline__ void
h6_server_fin_this(h6_svr_t *s)
{
    // ensure self had been removed from list
    TRACE_DEBUG("h6 server objs(%p) destroyed...\r\n", s);
}

static __inline__ void
on_h6_basic_server_fin(obj_t *p)
{
	h6_svr_t *s = (h6_svr_t*)p;

    TRACE_ENTER_FUNCTION;
    
	if (s->ops && s->ops->fin)
	{
	    (*s->ops->fin)(s);
	}

	h6_server_fin_this(s);

    TRACE_EXIT_FUNCTION;
}

h6_svr_t *
h6_server_alloc(uint32_t size, h6_svr_ops *ops, void *u, const char *name)
{
	int32_t err;
	h6_svr_t *s;

    TRACE_ENTER_FUNCTION;
    
	if (size < sizeof(h6_svr_t))
	{
        TRACE_EXIT_FUNCTION;
		return NULL;
	}
    
	s = (h6_svr_t *)obj_new(size, on_h6_basic_server_fin, name);
	if (s)
	{
		h6_server_init_this(s);
		s->ops = ops;

		if (ops && ops->init)
		{
			err = (*ops->init)(s, u);
			if (err)
			{
				s->ops = NULL;
				obj_unref((obj_t *)s);	//@{fin_this()}

                TRACE_EXIT_FUNCTION;
                
				return NULL;
			}
		}
        else
        {
            TRACE_ERROR(
                "ops->init not defined in %s, cann't init server object.\r\n", 
                __FUNCTION__);
            obj_unref((obj_t *)s);

            TRACE_EXIT_FUNCTION;
            return NULL;
        }
	}

    TRACE_EXIT_FUNCTION;
    
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
h6_server_self_kill(h6_svr_t *s)
{
    // nothing to do at this moment
    assert(s->sched == NULL);
}

static __inline__ void
h6_server_kill(h6_svr_t *s)
{   
	if (s->ops && s->ops->kill)
	{
	    (*s->ops->kill)(s);
	}

    h6_server_self_kill(s);
}

void
h6_server_kill_unref(h6_svr_t *s)
{
    TRACE_ENTER_FUNCTION;
    
    h6_server_kill(s);
    h6_server_unref(s);

    TRACE_EXIT_FUNCTION;
}

int32_t
h6_server_add_client(h6_svr_t *s, void *u)
{
    int32_t err = -EINVAL;

    TRACE_ENTER_FUNCTION;
    
    if (s->ops && s->ops->add_client)
    {
        err = (*s->ops->add_client)(s, u);
    }

    TRACE_EXIT_FUNCTION;
    
    return err;
}

int32_t
h6_server_set_sched(h6_svr_t *svr, h6_scher_t *sched)
{
    if (!svr->sched)
    {
        svr->sched = sched;
        return 0;
    }
    else
    {
        TRACE_ERROR("svr->sched has been set, can set again.\r\n", __FUNCTION__);        
        return -EINVAL;
    }
}


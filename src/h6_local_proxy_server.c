#include <assert.h>
#include <stdlib.h>
#include "h6_listener.h"
#include "h6_local_proxy_server.h"
#include "trace.h"

static int32_t
h6_local_proxy_svr_init(h6_svr_t *svr, void *u)
{
    h6_local_proxy_svr_t *s = (h6_local_proxy_svr_t *)svr;

    s->run = H6_FALSE;
    
    s->lsn_set = lsn_set_new();
    if (s->lsn_set == NULL)
    {        
        return -1;
    }
    
    s->cli_set = client_set_new();
    if (s->cli_set == NULL)
    {
        obj_unref(s->lsn_set);
    }
        
    
    return 0;
}

static void
h6_local_proxy_svr_finalize(h6_svr_t *svr)
{
    h6_local_proxy_svr_t *s = (h6_local_proxy_svr_t *)svr;
    
    assert(list_empty(&s->lsn_set->lsn_list));
    assert(list_empty(&s->cli_set->c_list));
}

static void
h6_local_proxy_svr_kill(h6_svr_t *svr)
{
    h6_local_proxy_svr_t *s = (h6_local_proxy_svr_t *)svr;

    if (s->lsn_set)
        lsn_set_kill_unref(s->lsn_set);

    if (s->cli_set)
        client_set_kill_unref(s->cli_set);        
    
    if (svr->sched)
    {
        h6_sched_free(svr->sched);
        svr->sched = NULL;
    }
}

static int32_t
h6_local_proxy_svr_add_client(h6_svr_t *svr, void *u)
{
    h6_local_proxy_svr_t *local_svr;

    local_svr = (h6_local_proxy_svr_t *)svr;

    return client_set_add(local_svr->cli_set, (client_t *)u);
}

static h6_svr_ops h6_svr_ops_impl =
{
	.init				= h6_local_proxy_svr_init,
	.fin				= h6_local_proxy_svr_finalize,
	.kill				= h6_local_proxy_svr_kill,
    .add_client         = h6_local_proxy_svr_add_client
};

h6_local_proxy_svr_t *
h6_local_proxy_server_alloc(uint32_t size, h6_local_proxy_svr_ops *ops, void *u, const char *name)
{
    return (h6_local_proxy_svr_t *)h6_server_alloc(size, &h6_svr_ops_impl, u, name);
}

__inline__ h6_bool_t
h6_local_proxy_server_is_running(h6_local_proxy_svr_t *svr)
{
    return svr->run;
}


int32_t
h6_local_proxy_server_bind_port(h6_local_proxy_svr_t *svr, uint16_t port)
{
    listener_t *lsn;
    int32_t    err;

    if (!h6_local_proxy_server_is_running(svr))
    {
        TRACE_ERROR("Server isn't running, can not add listenning port now.\r\n");
        return -EINVAL;
    }
    
    lsn = alloc_h6_listener(NULL);
    listener_set_owner(lsn, svr);
    
    err = listener_bind(lsn, port);
    if (err)
    {
        obj_unref((obj_t *)lsn);
        return err;
    }

    assert(svr->lsn_set);
    lsn_set_add(svr->lsn_set, lsn);

    return 0;     
}

static __inline__ listener_t *
find_listener_by_port(lsn_set_t *set, uint16_t port)
{
    struct list_head *l;
    listener_t *lsn;
    h6_listener_t *h6_lsn;
    
    list_for_each(l, &set->lsn_list)
    {
        lsn = list_entry(l, listener_t, list_node);
        h6_lsn = (h6_listener_t *)lsn;

        if (h6_lsn->port == port)
            return lsn;
    }

    return NULL;
}

void
h6_local_proxy_server_remove_port(h6_local_proxy_svr_t *svr, uint16_t port)
{
    listener_t *lsn;

    TRACE_ENTER_FUNCTION;
    
    assert(svr->lsn_set);
    lsn = find_listener_by_port(svr->lsn_set, port);
    if (lsn)
    {
        lsn_set_del(svr->lsn_set, lsn);        
        obj_unref(lsn);
    }

    TRACE_EXIT_FUNCTION;
}

int32_t
h6_local_proxy_server_run(h6_local_proxy_svr_t *svr, int32_t loops)
{
    h6_scher_t *sched;
    int32_t  err = -EINVAL;
    
    if (svr->run == H6_TRUE)
        return -EINVAL;
        
    sched = h6_sched_new(loops);
    if (sched)        
    {
        err = h6_server_set_sched((h6_svr_t *)svr, sched);
        if (!err)
            svr->run = H6_TRUE;
        else
            h6_sched_free(sched);
        
        return err;        
    }                

    return -EINVAL;    
}

void
h6_local_proxy_server_kill_unref(h6_local_proxy_svr_t *svr)
{
    h6_server_kill_unref((h6_svr_t *)svr);
}




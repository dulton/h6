#include <assert.h>
#include <stdlib.h>
#include "h6_local_proxy_server.h"

static int32_t
h6_local_proxy_svr_init(h6_svr_t *svr, void *u)
{
    h6_lsn_svr_t *s = (h6_svr_t)svr;

    s->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (s->lock == NULL)
        return -1;
    
    pthread_mutex_init(s->lock, NULL);

    s->lsn_set = lsn_set_new();
    if (s->lsn_set == NULL)
    {
        pthread_mutex_destroy(s->lock);
        free(s->lock);
        
        return -1;
    }

    s->proto_watch = NULL;
    
    return 0;
}

static void
h6_local_proxy_svr_finalize(h6_svr_t *svr)
{
    h6_lsn_svr_t *s = (h6_svr_t)svr;

    if (s->lsn_set)
        lsn_set_kill_unref(s->lsn_set);
        
    if (s->lock)
    {
        pthread_mutex_destroy(s->lock);
        free(s->lock);
    }    
}

static void
h6_local_proxy_svr_kill(h6_svr_t *svr)
{
    
}



static h6_svr_ops h6_svr_ops_impl =
{
	.init				= h6_local_proxy_svr_init,
	.fin				= h6_local_proxy_svr_finalize,
	.kill				= h6_local_proxy_svr_kill
};

h6_lsn_svr_t *
h6_local_proxy_server_alloc(uint32_t size, h6_lsn_svr_ops *ops, void *u, const char *name)
{
	int32_t err;
	h6_lsn_svr_t *svr;

	if (size < sizeof(h6_lsn_svr_t))
		return NULL;
    
    return (h6_lsn_svr_t *)h6_server_alloc(size, &h6_svr_ops_impl, u, name);
}

int32_t
h6_local_proxy_server_bind_port(h6_lsn_svr_t *svr, uint16_t port)
{
    listener_t *lsn;
    int32_t    err;
    
    lsn = alloc_h6_listener(NULL);

    err = listener_bind(lsn, port);
    if (err)
    {
        obj_unref((obj_t *)lsn);
        return err;
    }

    listener_set_owner(lsn);
    listener_start(lsn);

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
h6_local_proxy_server_remove_port(h6_lsn_svr_t *svr, uint16_t port)
{
    listener_t *lsn;

    assert(svr->lsn_set);
    lsn = find_listener_by_port(svr->lsn_set, port)
    if (lsn)
    {
        lsn_set_del(svr->lsn_set, lsn);        
        obj_unref(lsn);
    }
}



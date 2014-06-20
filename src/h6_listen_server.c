#include "h6_listen_server.h"

static int32_t
h6_listen_svr_init(h6_svr_t *svr, void *u)
{
    h6_local_proxy_svr_t *s = (h6_svr_t)svr;

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
h6_listen_svr_finalize(h6_svr_t *svr)
{
    h6_local_proxy_svr_t *s = (h6_svr_t)svr;

    if (s->lsn_set)
        lsn_set_kill_unref(s->lsn_set);
        
    if (s->lock)
    {
        pthread_mutex_destroy(s->lock);
        free(s->lock);
    }    
}

static void
h6_listen_svr_kill(h6_svr_t *svr)
{
    
}



static h6_svr_ops h6_svr_ops_impl =
{
	.init				= h6_listen_svr_init,
	.fin				= h6_listen_svr_finalize,
	.kill				= h6_listen_svr_kill
};

h6_local_proxy_svr_t *
h6_listen_server_alloc(uint32_t size, h6_local_proxy_svr_ops *ops, void *u, const char *name)
{
	int32_t err;
	h6_local_proxy_svr_t *svr;

	if (size < sizeof(h6_local_proxy_svr_t))
		return NULL;
    
    return (h6_local_proxy_svr_t *)h6_server_alloc(size, &h6_svr_ops_impl, u, name);
}

int32_t
h6_listen_server_bind_port(h6_local_proxy_svr_t *svr, uint16_t port)
{
    int32_t fd;
    
    fd = unix_sock_bind(L4_TCP, 0, htons(port), 0);
    if (fd < 0)
    {
        TRACE_ERROR("unix_sock_bind() failed, err:'%d' @ port(%u).", fd, port);
        return fd;
    }

    unix_sock_tcp_listen(fd);
    
     
}



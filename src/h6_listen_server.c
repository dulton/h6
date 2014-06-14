#include "h6_listen_server.h"

static int32_t
h6_listen_svr_init(h6_svr_t *svr, void *u)
{
}

static void
h6_listen_svr_finalize(h6_svr_t *svr)
{
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

h6_lsn_svr_t *
h6_listen_server_alloc(uint32_t size, h6_lsn_svr_ops *ops, void *u, const char *name)
{
	int32_t err;
	h6_lsn_svr_t *s;

	if (size < sizeof(h6_lsn_svr_t))
		return NULL;
    
    //...

	return s;       
}


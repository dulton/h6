#ifndef __H6_LOCAP_PROXY_SERVER__
#define __H6_LOCAP_PROXY_SERVER__

#include <pthread.h>
#include "listener_set.h"
#include "h6_basic_server.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __h6_local_proxy_server       h6_lsn_svr_t;
typedef struct __h6_local_proxy_server_ops   h6_lsn_svr_ops;

struct __h6_local_proxy_server
{
    h6_svr_t        __upper;

    pthread_mutex_t *lock;
    lsn_set_t       *lsn_set;
    
    h6_lsn_svr_ops  *ops;
};

struct __h6_local_proxy_server_ops
{
	int32_t (*init)(h6_lsn_svr_t *svr);
	void	(*fin)(h6_lsn_svr_t *svr);    
};

h6_lsn_svr_t *
h6_local_proxy_server_alloc(uint32_t size, h6_lsn_svr_ops *ops, void *u, const char *name);

int32_t
h6_local_proxy_server_bind_port(h6_lsn_svr_t *svr, uint16_t port);

void
h6_local_proxy_server_remove_port(uint16_t port);

#ifdef __cplusplus
}
#endif

#endif

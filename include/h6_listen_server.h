#ifndef __H6_LISTEN_SERVER__
#define __H6_LISTEN_SERVER__

#include "h6_basic_server.h"
#include "listener_set.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __h6_listen_server       h6_lsn_svr_t;
typedef struct __h6_listen_server_ops   h6_lsn_svr_ops;

struct __h6_listen_server
{
    h6_svr_t        __upper;
    lsn_set_t       *lsn_set;

    h6_lsn_svr_ops  *ops;
};

struct __h6_listen_server_ops
{
    int32_t (*bind_port)(h6_lsn_svr_t *svr, uint16_t port);
};

h6_lsn_svr_t *
h6_listen_server_alloc(uint32_t size, h6_lsn_svr_ops *ops, void *u, const char *name);

int32_t
h6_listen_server_bind_port(h6_lsn_svr_t *svr, uint16_t port);


#ifdef __cplusplus
}
#endif

#endif

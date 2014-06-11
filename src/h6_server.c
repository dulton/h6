#include <stdlib.h>
#include "h6_server.h"
#include "h6_listener.h"

#define DEFAULT_LOOPS 4

h6_server *
h6_create_server()
{
    h6_server *server;
    
    server = (h6_server *)calloc(1, sizeof(h6_server));
    if (server)
    {
        client_set_init(&server->src_cs);
        client_set_init(&server->dst_cs);
        
        relay_set_init(&server->rs);
        server->sched = h6_sched_new(DEFAULT_LOOPS);
        server->ls = alloc_h6_listener();
        listener_set_owner(ls, server);
    }
    
    return server;
}

int32_t
h6_server_bind_port(h6_server *svr, uint16_t port)
{
    return -1;
}

int32_t
start_h6_server(h6_server *svr)
{
    return -1;
}

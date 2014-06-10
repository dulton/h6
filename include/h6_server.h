/*
 * H6: copyright (c) 2014, Shiyong Zhang<shiyong.zhang.cn@gmail.com>
 * A simple relay server application framework.
*/


#ifndef __H6_SERVER_H__
#define __H6_SERVER_H__

#include "h6_sched.h"
#include "h6_client_set.h"
#include "h6_relay_set.h"
#include "listener.h"

#ifdef __cplusplus
extern "C" {
#endif

enum svr_type
{
    svr_unknown = 0,
    svr_relay
    // to be added...
};

typedef struct __h6_server h6_server;
struct __h6_server
{
    client_set   src_cs;
    client_set   dst_cs;
    
	relay_set    rs;	    /* relay set */
    
	listener_t   *ls;	    /* listener set */
	h6_scher_t   *sched;
};

h6_server *h6_create_server();
int32_t h6_server_bind_port(h6_server *svr, uint16_t port);
int32_t start_h6_server(h6_server *svr);

#ifdef __cplusplus
}
#endif

#endif	//__H6_SERVER_H__

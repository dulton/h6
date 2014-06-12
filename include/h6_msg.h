#ifndef _H6_MSG_H_
#define _H6_MSG_H_

#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Initiator scenario:
 * setup channel => data
 *
 * Receiver scrnario
 * Register => heart beat(wait...) => setup channel => data
 */
// METHOD:H6_SETUP_CHANNEL
// 
// live_id:
// dst_id:
// dst_host:
// dst_port:

// METHOD:H6_SETUP_CHANNEL_RESPONSE
// result_code:
// 

// H6_REGISTER:
// self_id:
// host: 

// H6_REGISTER_ACK:
// result_code:
// live_id: 临时分配的生命ID，以后用这个ID与服务器通讯
// keep_alive: heart beat period between Proxy Agent and Server

// H6_HEART_BEAT:
// self_id: 
// keep_alive:

// H6_HEART_BEAT_ACK
// result_code:
// server_time:




#ifdef __cplusplus
}
#endif

#endif

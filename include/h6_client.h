#ifndef _H6_CLIENT_H_
#define _H6_CLIENT_H_

#include <stdint.h>
#include "network_client.h"
#include "proto_parser.h"
#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    H6_UNKNOWN,
        
    // h6 client msg type handled by H6 server    
    H6_SETUP_CHANNEL,   // src client request to setup channel
    H6_RESPONSE,        // server respond msg to src/dst client
    H6_DATA,            // src/dst client send/respond data

    H6_REGISTER,        // dst peer registered in server
    H6_HEART_BEAT        // heartbeat between server and dst while no message 
                        // initiated from src
};

typedef enum
{
    H6_STATE_INIT = 0,
    H6_STATE_READY,
    H6_STATE_TRANSFERING,
    H6_STATE_UNKNOWN
}h6_client_state;

typedef struct __h6_client      h6_client;
typedef struct __h6_client_ops  h6_client_ops;

struct __h6_client
{
	network_client  client_base;
	h6_client_ops   *ops;
};

struct __h6_client_ops
{
	int32_t (*init)(h6_client *hc);
	void	(*fin)(h6_client *hc);
	proto_parser *(*create_proto_parser)(h6_client *hc);
	void 	(*release_proto_parser)(h6_client *hc, proto_parser *parser);
	void	(*kill)(h6_client *hc);

	uint32_t (*recognize)(h6_client *hc, msg_t *req);
	int32_t	(*on_setup_channel)(h6_client *hc, msg_t *req);
    int32_t (*on_response(h6_client *hc, msg_t req);
	int32_t (*on_data)(h6_client *hc, msg_t *req);
    int32_t (*on_register)(h6_client *hc, msg_t *req);
    int32_t (*on_heart_beat)(h6_client *hc, msg_t *req);
    
	void    (*on_closed)(h6_client *hc);
	void    (*on_error)(h6_client *hc, int32_t err);
};


#ifdef __cplusplus
}
#endif


#endif

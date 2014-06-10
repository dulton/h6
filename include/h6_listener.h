#ifndef __H6_LISTENER_H__
#define __H6_LISTENER_H__

#include "listener.h"
#include "h6_ev.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __h6_listener_ h6_listener_t;
typedef struct __h6_listener_ops h6_listener_ops;

struct __h6_listener_
{
	listener_t obj_base;		/* private inheritance */

	uint16_t port;
	uint32_t host;

	int32_t sock;
	h6_ev_t *event;

	h6_listener_ops *ops;
	void *user_data;
};


struct __h6_listener_ops
{
	int32_t (*init)(h6_listener_t *lt);
	void	(*fin)(h6_listener_t *lt);

	client_t* (*new_cli)(h6_listener_t *lt, int32_t sock);
};


listener_t *alloc_h6_listener();

#ifdef __cplusplus
}
#endif

#endif
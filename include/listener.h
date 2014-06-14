#ifndef _LISTENER_H_
#define _LISTENER_H_

#include "client.h"

typedef struct __listener       listener_t;
typedef struct __listener_ops   listener_ops;

struct __listener
{
    obj_t               __super;

    struct list_head    list_node;

	listener_ops        *ops;
	void                *server;	/* pointer owner, which should be a server */
};

struct __listener_ops
{
	int32_t     (*init)(listener_t *l, void *u);
	void 	    (*fin)(listener_t *l);
	int32_t     (*set_port)(listener_t *l, uint16_t port);
	int32_t     (*run)(listener_t *l);
	client_t*   (*new_cli)(listener_t *l, void *parm);
};

listener_t *listener_alloc(uint32_t size, listener_ops *ops, void *u);
int32_t listener_bind(listener_t *l, uint16_t port);
int32_t listener_start(listener_t *l);
client_t *listener_generate(listener_t *l, void *parm);

void listener_set_owner(listener_t *l, void *p);
void *listener_get_owner(listener_t *l);


#endif

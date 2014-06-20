#ifndef _LISTENER_H_
#define _LISTENER_H_

#include "client.h"

typedef struct __listener       listener_t;
typedef struct __listener_ops   listener_ops;

typedef struct __listener_set       lsn_set_t;
typedef struct __listener_set_ops   lsn_set_ops;

struct __listener
{
    obj_t               __super;

    struct list_head    list_node;

    lsn_set_t           *lset;

	listener_ops        *ops;
	void                *server;	/* pointer owner, which should be a server */
};

struct __listener_ops
{
	int32_t     (*init)(listener_t *l, void *u);
	void 	    (*fin)(listener_t *l);
	int32_t     (*set_port)(listener_t *l, uint16_t port);
    void        (*kill)(listener_t *l);
	client_t*   (*new_cli)(listener_t *l, void *parm);
};

listener_t *
listener_alloc(uint32_t size, listener_ops *ops, void *u);

void
listener_kill_unref(listener_t *l);

int32_t
listener_bind(listener_t *l, uint16_t port);

client_t *
listener_generate(listener_t *l, void *parm);

void
listener_set_owner(listener_t *l, void *p);

void *
listener_get_owner(listener_t *l);

// =====================================================================

struct __listener_set
{
    obj_t               __upper;

	struct list_head    lsn_list;    
	pthread_mutex_t     *lock;

    lsn_set_ops         *ops;
};

struct __listener_set_ops
{
	int32_t (*init)(lsn_set_t *set, void *u);
	void	(*fin)(lsn_set_t *set);
	void	(*kill)(lsn_set_t *set);
};

lsn_set_t *lsn_set_new();
lsn_set_t *lsn_set_ref(lsn_set_t *c);
void lsn_set_unref(lsn_set_t *c);
void lsn_set_kill_unref(lsn_set_t *c);

int32_t lsn_set_add(lsn_set_t *lset, listener_t *l);
void lsn_set_del(lsn_set_t *lset, listener_t *l);

#endif

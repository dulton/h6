#ifndef _CLIENT_SET_H_
#define _CLIENT_SET_H_

#include <pthread.h>
#include "client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __client_set     client_set_t;
typedef struct __client_set_ops client_set_ops;

struct __client_set
{
    obj_t            __supper;

	pthread_mutex_t  *lock;
    struct list_head c_list;
    
    client_set_ops   *ops;
};

struct __client_set_ops
{
	int32_t (*init)(client_set_t *c, void *u);
	void	(*fin)(client_set_t *c);
	void	(*kill)(client_set_t *c);
};

client_set_t* client_set_new();
client_set_t *client_set_ref(client_set_t *c);
void client_set_unref(client_set_t *c);
void client_set_kill_unref(client_set_t *c);

int32_t client_set_add(client_set_t *cs, client_t *c);
void client_set_del(client_set_t *cs, client_t *c);


#ifdef __cplusplus
}
#endif

#endif


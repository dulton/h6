#ifndef _CLIENT_SET_H_
#define _CLIENT_SET_H_

#include <pthread.h>
#include "client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __client_set client_set;
struct __client_set
{
	pthread_mutex_t  *lock;
	struct list_head c_list;
};

void client_set_init(client_set *cs);
int32_t client_set_add(client_set *cs, client_t *c);
void client_set_del(client_set *cs, client_t *c);


#ifdef __cplusplus
}
#endif

#endif


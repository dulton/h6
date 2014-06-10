#ifndef __H6_RELAY_SET_H__
#define __H6_RELAY_SET_H__

#include <pthread.h>
#include "list.h"
#include "client.h"
#include "h6_relay_pair.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct __relay_set relay_set;
struct __relay_set
{
	pthread_mutex_t  *lock;
	struct list_head *pair_list;
};

void relay_set_init(relay_set *rs);
int32_t relay_set_add(relay_set *rs, relay_pair_t *pair);
void relay_set_del(relay_set *rs, relay_pair_t *pair);

#ifdef __cplusplus
}
#endif

#endif

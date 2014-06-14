#ifndef __LISTENER_SET_H__
#define __LISTENER_SET_H__

#include <pthread.h>
#include <stdint.h>
#include "obj.h"
#include "list.h"

typedef struct __listener_set       lsn_set_t;
typedef struct __listener_set_ops   lsn_set_ops;

struct __listener_set
{
    obj_t               __upper;

	struct list_head    lsn_list;    
	pthread_mutex_t     *lock;

    lsn_set_ops         *ops;
};

lsn_set_t *lsn_set_new();
lsn_set_t *lsn_set_ref(lsn_set_t *c);
void lsn_set_unref(lsn_set_t *c);
void lsn_set_kill_unref(lsn_set_t *c);

int32_t lsn_set_add(lsn_set_t *lset, listener_t *l);
void lsn_set_del(lsn_set_t *lset, listener_t *l);


#endif

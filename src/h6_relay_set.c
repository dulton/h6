#include <stdlib.h>
#include <errno.h>
#include "h6_relay_set.h"

void
relay_set_init(relay_set *rs)
{
	rs->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(rs->lock, NULL);
    
    rs->pair_list = (struct list_head *)malloc(sizeof(struct list_head));
	INIT_LIST_HEAD(rs->pair_list);
}

static __inline__ int32_t
__relay_set_add(relay_set *rs, relay_pair_t *pair)
{
    int32_t err = -EINVAL;
    
    if (list_empty(pair->list_node))
    {
        err = 0;
        pair->set = rs;
        list_add(pair->list_node, rs->pair_list);
    }
    
    return err;
}

int32_t
relay_set_add(relay_set *rs, relay_pair_t *pair)
{
    int32_t error;
    
    pthread_mutex_lock(rs->lock);
    error = __relay_set_add(rs, pair);
    pthread_mutex_unlock(rs->lock);
    
    return error;
}

static __inline__ int32_t 
__relay_set_del(relay_set *rs, relay_pair_t *pair)
{
    int32_t err = -EINVAL;
    
    if (!list_empty(pair->list_node))
    {
        err = 0;
        pair->set = NULL;
        list_del_init(pair->list_node);
    }
    
    return err;
}

void
relay_set_del(relay_set *rs, relay_pair_t *pair)
{    
    pthread_mutex_lock(rs->lock);
    __relay_set_del(rs, pair);
    pthread_mutex_unlock(rs->lock);
    
    return;
}
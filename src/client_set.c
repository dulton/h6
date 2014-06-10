#include <stdlib.h>
#include <errno.h>
#include "client_set.h"

void client_set_init(client_set *cs)
{
	cs->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(cs->lock, NULL);
    
	INIT_LIST_HEAD(&cs->c_list);
}


static __inline__ int32_t
__client_set_add(client_set *cs, client_t *c)
{
	int32_t err = -EINVAL;

	if (list_empty(&c->list_node))
	{
		err = 0;
		list_add(&c->list_node, &cs->c_list);
	}

	return err;
}


int32_t client_set_add(client_set *cs, client_t *c)
{
	int32_t err;

	pthread_mutex_lock(cs->lock);
	err = __client_set_add(cs, c);
	pthread_mutex_unlock(cs->lock);

	return err;
}


static __inline__ int32_t
__client_set_del(client_set *cs, client_t *c)
{
	int32_t err = -EINVAL;

	if (!list_empty(&c->list_node))
	{
		err = 0;
		list_del_init(&c->list_node);
	}

	return err;
}


void client_set_del(client_set *cs, client_t *c)
{
	pthread_mutex_lock(cs->lock);
	__client_set_del(cs, c);
	pthread_mutex_unlock(cs->lock);
}



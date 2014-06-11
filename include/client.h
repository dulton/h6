#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <stdint.h>
#include <pthread.h>
#include "obj.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __client client_t;
typedef struct __client_ops client_ops;

struct __client
{
    obj_t       __super;

    struct list_head list_node;
    
	void        *sched;
	client_ops  *ops;
};

struct __client_ops
{
	int32_t (*init)(client_t *c, void *u);
	void	(*fin)(client_t *c);
	void	(*kill)(client_t *c);
	int32_t (*attach)(client_t *c, void *loop);
};

client_t *client_alloc(uint32_t size, client_ops *ops, void *u, const char *name);
client_t *client_ref(client_t *c);
void client_unref(client_t *c);
void client_kill_unref(client_t *c);
int32_t client_attach(client_t *c, void *sched);


#ifdef __cplusplus
}
#endif

#endif	//__CLIENT_H__

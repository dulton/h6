#ifndef __H6_BASIC_SERVER_H__
#define __H6_BASIC_SERVER_H__

#include <stdint.h>
#include "obj.h"
#include "h6_sched.h"

typedef struct __h6_basic_server        h6_svr_t;
typedef struct __h6_basic_server_ops    h6_svr_ops;
    
struct __h6_basic_server
{
    obj_t       __supper;
    h6_scher_t  *sched;

    h6_svr_ops  *ops;
};

struct __h6_basic_server_ops
{
	int32_t (*init)(h6_svr_t *svr, void *u);
	void	(*fin)(h6_svr_t *svr);
	void	(*kill)(h6_svr_t *svr);
	int32_t (*set_sched)(h6_svr_t *svr, h6_scher_t **sched);
    int32_t (*add_client)(h6_svr_t *svr, void *u);
};


h6_svr_t *h6_server_alloc(uint32_t size, h6_svr_ops *ops, void *u, const char *name);
h6_svr_t *H6_server_ref(h6_svr_t *s);
void h6_server_unref(h6_svr_t *s);
void h6_server_kill_unref(h6_svr_t *s);

int32_t h6_server_add_client(h6_svr_t *s, void *u);

#endif

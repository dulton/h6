#ifndef _PROTO_WATCH_H_
#define _PROTO_WATCH_H_

#include <pthread.h>
#include <stdint.h>
#include "list.h"
#include "h6_ev.h"
#include "proto_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*proto_watch_on_fin)(void *u);
typedef struct __proto_watch proto_watch;
typedef struct __proto_watch_ops proto_watch_ops;

struct __proto_watch
{
	h6_ev_t             __super;
	int32_t             watch_fd;
	proto_parser        *parser;
	proto_watch_on_fin  fin;
	uint32_t            state;
	uint32_t            window_size;
	uint32_t            connecting;
	void                *sched;
    mb_t                *write_buffer;
	struct list_head    backlog_list;
	uint32_t            backlog_size;
	pthread_mutex_t     *lock;
    
	proto_watch_ops     *ops;
};

struct __proto_watch_ops
{
	proto_parser *(*create_proto_parser)(void *u);
	void    (*release_proto_parser)(proto_parser *parser, void *u);
	int32_t (*proto_msg_sent)(proto_watch *w, uint32_t seq, void *u);	//TODO
	int32_t (*proto_msg_recv)(proto_watch *w, msg_t *m, void *u);
	void    (*conn_closed)(proto_watch *w, void *u);
	void    (*conn_error)(proto_watch *w, int32_t err, void *u);
};

proto_watch *proto_watch_new(void *io, int32_t timeout, proto_watch_ops *ops,
	void *u, proto_watch_on_fin on_finalize);

proto_watch *proto_watch_ref(proto_watch *w);
void proto_watch_unref(proto_watch *w);
void proto_watch_kill_unref(proto_watch *w);

int32_t proto_watch_set_window(proto_watch *w, uint32_t win_size);

int32_t proto_watch_attach(proto_watch *w, void *sched);

int32_t proto_watch_write(proto_watch *w, msg_t *m, uint32_t seq, uint32_t flags);
int32_t proto_watch_write_mb(proto_watch *w, mb_t *mb, uint32_t flags);

int32_t proto_watch_writeable(proto_watch *w, uint32_t size);
int32_t proto_watch_set_dst(proto_watch *w, uint8_t *ip, int32_t port);

#ifdef __cplusplus
}
#endif

#endif

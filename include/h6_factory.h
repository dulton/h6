#ifndef _H6_FACTORY_H_
#define _H6_FACTORY_H_

#include "listener.h"
#include "client.h"
#include "proto_parser.h"
#include "h6_sched.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __h6_factory h6_factory;
struct __h6_factory
{
	uint8_t *name;	//@{factory name}
	uint32_t id;	//@{factory id}

	int32_t (*init_factory)(h6_factory *h6_f);

	listener_t *(*create_listener)(h6_factory *h6_f);
	void (*destroy_listener)(h6_factory *h6_f, listener_t *l);

	h6_scher_t *(*create_scheduler)(h6_factory *h6_f);
	void (*destroy_scheduler)(h6_factory *h6_f, h6_scher_t *sched);

	proto_parser *(*create_client_proto_parser)(h6_factory *h6_f);
	void (*destroy_client_proto_parser)(h6_factory *h6_f, proto_parser *p);
};

//{@get a factory object by id}
h6_factory *get_tr_factory(uint32_t factory_id);

//@{init factory}
int32_t init_factory(h6_factory *factory);

//@{factory create/destroy client listener}
listener_t *factroy_create_h6_listener(h6_factory *factory);
void factory_destroy_h6_listener(h6_factory *factory, listener_t *l);

//@{factory create/destroy event scheduler}
h6_scher_t *factory_create_scheduler(h6_factory *factory);
void factory_destroy_scheduler(h6_factory *factory, h6_scher_t *sched);

proto_parser *factory_create_client_proto_parser(h6_factory *factory);
void factory_destroy_client_proto_parser(h6_factory *factory, proto_parser *p);

#ifdef __cplusplus
}
#endif

#endif

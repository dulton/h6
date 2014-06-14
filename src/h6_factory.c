#include "h6_factory.h"
#include "trace.h"

extern h6_factory h6_server_factory;

static h6_factory *factory_array[] = 
{
	&h6_server_factory
};

//{@get a factory object by id}
h6_factory *
get_tr_factory(uint32_t factory_id)
{
	int32_t f = sizeof(factory_array)/sizeof(factory_array[0]);

	if (factory_id >= f)
		return NULL;

	return factory_array[factory_id];
}

//@{init factory}
int32_t
init_factory(h6_factory *factory)
{
	if (!factory->init_factory)
		return 0;

	return (*factory->init_factory)(factory);

}

//@{factory create/destroy client listener}
listener_t *
factroy_create_h6_listener(h6_factory *factory)
{
	if (!factory->create_listener)
		return NULL;
	return (*factory->create_listener)(factory);
}

void
factory_destroy_h6_listener(h6_factory *factory, listener_t *l)
{
	if (!factory->destroy_listener)
    	{   
		TRACE_ERROR("Don't define destroy_listener function, cann't destroy listener(%p)\r\n", l);
        	return;
    	}

	(*factory->destroy_listener)(factory, l);

}

//@{factory create/destroy event scheduler}
h6_scher_t *
factory_create_scheduler(h6_factory *factory)
{
	if (!factory->create_scheduler)
		return NULL;
    
	return (*factory->create_scheduler)(factory);

}

void
factory_destroy_scheduler(h6_factory *factory, h6_scher_t *sched)
{
	if (!factory->destroy_scheduler)
    	{   
        	TRACE_ERROR("Don't define destroy_scheduler function, cann't destroy scheduler(%p)\r\n", sched);
        	return;
    	}
    
	(*factory->destroy_scheduler)(factory, sched);
}

proto_parser *
factory_create_client_proto_parser(h6_factory *factory)
{
	if (!factory->create_client_proto_parser)
		return NULL;
	return (*factory->create_client_proto_parser)(factory);

}

void
factory_destroy_client_proto_parser(h6_factory *factory, proto_parser *p)
{
	if (!factory->destroy_client_proto_parser)
	{
		TRACE_ERROR("Don't define destroy_client_proto_parser function, cann't destroy proto_parser(%p\r\n)", p);
		return;
	}
	
	(*factory->destroy_client_proto_parser)(factory, p);
}


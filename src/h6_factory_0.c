#include "h6_factory.h"
#include "h6_sched.h"

#define N_LOOPS 2
#define FACTORY_0_H6_SERVER	0

static h6_scher_t *
create_scheduler(h6_factory *tr_f)
{
	return h6_sched_new(N_LOOPS);
}

h6_factory h6_server_factory = 
{
    .name                           = (uint8_t *)("H6 Server Instance Factory"),
    .id                             = FACTORY_0_H6_SERVER,
    .init_factory                   = NULL,
    .create_listener		    = NULL,
    .create_scheduler		    = create_scheduler,
    .create_client_proto_parser	    = NULL,
    .destroy_client_proto_parser    = NULL    
};



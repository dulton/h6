#ifndef _H6_CLIENT_IMP_H_
#define _H6_CLIENT_IMP_H_

#include <pthread.h>
#include "client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __h6_ts_client h6_ts_client;	/* thread-safe client */
struct __h6_ts_client
{
	h6_client   __super;
	int32_t     killed;
	pthread_t   *lock;
};


client_t *h6_impl_client_new(uint32_t factory, void *sock);
client_t *h6_impl_ts_client_new(uint32_t factory, void *sock);
int32_t h6_impl_ts_client_killed(rtsp_ts_client *rtc);



#ifdef __cplusplus
}
#endif

#endif

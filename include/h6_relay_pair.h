#ifndef __H6_RELAY_PAIR_H__
#define __H6_RELAY_PAIR_H__

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __relay_set relay_set;
typedef struct __relay_pair relay_pair_t;
struct __relay_pair
{
    struct list_head *list_node;
    
    relay_set        *set;   // owner, should be a relay_set struct
    client_t         *src;   // communication initiator
    client_t         *end;   // communication receiver and respondents    
};


#ifdef __cplusplus
}
#endif

#endif // __H6_RELAY_PAIR_H__
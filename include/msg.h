#ifndef __MSG_H__
#define __MSG_H__

#include <stdint.h>
#include "obj.h"
#include "mem_block.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MT_MASK  0x03

typedef enum
{
	MT_01 = 0x01,
	MT_02 = 0x02,
	MT_03 = 0x03
}msg_type_t;

typedef struct __msg msg_t;
typedef struct __msg_ops msg_ops;

struct __msg
{
    obj_t       __super;
    msg_type_t  type;
};

struct __msg_ops
{
	msg_t *(*dup)(msg_t *m);
	uint32_t (*msg_size)(msg_t *m);
	mb_t *(*msg_to_mb)(msg_t *m);
};

int32_t register_msg_type(msg_type_t mt, msg_ops *ops); 

msg_t *msg_ref(msg_t *m);
void msg_unref(msg_t *m);

msg_t *msg_dup(msg_t  *m);
uint32_t msg_size(msg_t *m);

mb_t *msg_to_mb(msg_t *m);	//@{get msg_t data, and unref msg_t}


#ifdef __cplusplus
}
#endif

#endif

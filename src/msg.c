#include <assert.h>
#include <errno.h>
#include "msg.h"
#include "mem_block.h"

#define GET_MSG_OPS(msg) ops_array[msg->type & MT_MASK]
static msg_ops *ops_array[4] =  {NULL};


int32_t
register_msg_type(msg_type_t mt, msg_ops *ops)
{
	if (ops_array[mt])
		return -EEXIST;

	ops_array[mt] = ops;
	return 0;
}


msg_t *msg_ref(msg_t *m)
{
    if (m)
        obj_ref((obj_t *)m);

    return m;
}


void msg_unref(msg_t *m)
{
    if (m)
        obj_unref((obj_t *)m);
}


msg_t *msg_dup(msg_t  *m)
{
	msg_ops *ops;

	ops = GET_MSG_OPS(m);
	assert(ops);

	return (*ops->dup)(m);
}


uint32_t msg_size(msg_t *m)
{
	msg_ops *ops;

	ops = GET_MSG_OPS(m);
	assert(ops);

	return (*ops->msg_size)(m);
}


mb_t *msg_to_mb(msg_t *m)
{
	msg_ops *ops;

	ops = GET_MSG_OPS(m);
	assert(ops);

	return (*ops->msg_to_mb)(m);
}



#include "h6_client.h"

static __inline__ uint8_t *
h6_cmd_p(int32_t id)
{
	uint8_t *str[] = {
		"UNKNOW"
        ,"SETUP_CHANNEL"
        ,"RESPONSE"
        ,"DATA",
		,"REGISTER"
        ,"HEARTBEAT"
	};

	if (id < H6_UNKNOWN || id > H6_HEARTBEAT)
		return "ERROR";
    
	return str[id];
}

static int32_t
h6_client_init(network_client *c)
{
//	h6_client *hc = (h6_client*)c;
	return 0;
}

static void
h6_client_finalize(network_client *c)
{
	h6_client *hc = (h6_client*)c;

	if (hc->ops && hc->ops->fin)
	{
		(*hc->ops->fin)(hc);
	}
}

static __inline__ uint32_t
h6_client_recognize(h6_client *hc, msg_t *req)
{
	uint32_t id = H6_UNKNOWN;

	if (hc->ops && hc->ops->recognize)
	{
		id = (*hc->ops->recognize)(hc, req);
	}

	return id;
}

#define SET_H6_MSG_HANDLER(c) \
static __inline__ int32_t \
h6_client_handler_##c(h6_client *hc, msg_t *req) \
{\
	int32_t err = -EPERM; \
\
	if (hc->ops && hc->ops->on_##c) \
	{\
		err = (*hc->ops->on_##c)(hc, req); \
	}\
\
	return err;\
}

SET_H6_MSG_HANDLER(setup_channel)
SET_H6_MSG_HANDLER(response)
SET_H6_MSG_HANDLER(data)
SET_H6_MSG_HANDLER(register)
SET_H6_MSG_HANDLER(heart_beat)

#define CALL_HANDLER(c, hc, req) \
	 h6_client_handler_##c((hc), (req))


static int32_t
h6_client_handle_msg(network_client *c, msg_t *req)
{
	int32_t id, err = -EPERM;
	h6_client *hc = (h6_client*)c;

	id = h6_client_recognize(hc, req);
	TRACE_TRACE(
		"h6-client '%p' handle msg_t '%s'.",
		c, __str(rtsp_cmd_p(id)));

	switch (id)
	{
	case H6_SETUP_CHANNEL:
		err = CALL_HANDLER(setup_channel, hc, req);
		break;

	case H6_RESPONSE:
		err = CALL_HANDLER(response, hc, req);
		break;

	case H6_DATA:
		err = CALL_HANDLER(data, hc, req);
		break;

	case H6_REGISTER:
		err = CALL_HANDLER(register, hc, req);
		break;

	case H6_HEART_BEAT:
		err = CALL_HANDLER(heart_beat, hc, req);
		break;


	case H6_UNKNOWN:
		break;
	}

	if (!err)
	{
		TRACE_TRACE(
			"h6-client '%p' handle msg_t '%s' ok.",
			c, __str(h6_cmd_p(id)));
	}
	else
	{
		TRACE_WARNING(
			"h6-client '%p' handle msg_t '%s' failed!",
			c, __str(h6_cmd_p(id)));
	}
	return err;
}


static int32_t
h6_client_msg_sent(network_client *c, uint32_t seq)
{
#if 0
	int32_t err = -EINVAL;
	h6_client *hc = (h6_client*)c;

	if (hc->ops && hc->ops->on_msg_sent)
	{
		err = (*hc->ops->on_msg_sent)(hc, seq);
	}
#endif

	return 0;
} 


static void
h6_client_kill(network_client *c)
{
	h6_client *hc = (h6_client*)c;

	if (hc->ops && hc->ops->kill)
	{
		(*hc->ops->kill)(hc);
	}
}


static void
h6_client_conn_closed(network_client *c)
{
	h6_client *hc = (h6_client*)c;

	if (hc->ops && hc->ops->on_closed)
	{
		(*hc->ops->on_closed)(hc);
	}
}


static void
h6_client_conn_error(network_client *c, int32_t err)
{
	h6_client *hc = (h6_client*)c;

	if (hc->ops && hc->ops->on_error)
	{
		(*hc->ops->on_error)(hc, err);
	}
}

static network_client_ops h6_client_ops_impl =
{
	.init					= h6_client_init,
	.fin					= h6_client_finalize,
	.msg_recv				= h6_client_handle_msg,
	.msg_sent				= h6_client_msg_sent,
	.kill					= h6_client_kill,
	.closed					= h6_client_conn_closed,
	.error					= h6_client_conn_error
};


h6_client *
h6_client_new(uint32_t size, h6_client_ops *ops, uint32_t factory, void *io)
{
	int32_t err;
	h6_client *hc;

	if (size < sizeof(h6_client))
	{
		unix_sock_close((int32_t)io);
		return NULL;
	}

	hc = (h6_client*)network_client_new(size, &h6_client_ops_impl, factory, io);
	if (hc)
	{
		if (ops && ops->init)
		{
			err = (*ops->init)(hc);
			if (err)
			{
				network_client_kill_unref((network_client*)hc);
				return NULL;
			}
		}

		hc->ops = ops;
	}

	return hc;
}


int32_t
h6_client_send_msg(h6_client *hc, msg_t *res, uint32_t seq)
{
	int32_t err;
	network_client *nc = (network_client*)hc;

	err = network_client_send_msg(nc, res, seq, 0);
	if (err < 0)
	{
		TRACE_WARNING(
			"h6_client_send_msg()->network_client_send_msg() failed, "
			"err:'%d'.", err);
	}

	return err;
}





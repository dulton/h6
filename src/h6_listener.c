#include <errno.h>
#include "unix_sock.h"
#include "h6_listener.h"
#include "h6_server.h"
#include "h6_def.h"

static h6_bool_t
h6_listener_on_event(h6_ev_t *ev, int revents, void *user_data)
{
	h6_listener_t *lt;
	int32_t sock;
	client_t *cli;
	h6_server *server;

	lt = (h6_listener_t*)h6_ev_u(ev);

	if (revents & EV_TIMER)
	{
		TRACE_TRACE("=== LISTEN WAITING... ===");
		return H6_TRUE;
	}

	sock = unix_sock_tcp_accept(lt->sock);
	if (sock < 0 && sock != -EAGAIN)
	{
		TRACE_ERROR(
			"h6_listener_on_event()->unix_sock_tcp_accept() failed.\n"
		);
		return H6_FALSE;
	}

	cli = listener_generate((listener_t*)lt, (void*)sock);
	if (cli)
	{
		TRACE_TRACE(
			"Generate new client, sock:'%d'.", sock
		);

		server = (h6_server *)listener_get_owner((listener_t *)lt);

		client_attach(cli, server->sched);
		TRACE_TRACE("Generate attached client, sock:'%d'", sock);
	}
	
	return H6_TRUE;
}

static int32_t
h6_listener_init(listener_t *l, void *u)
{
	int32_t err;
	h6_listener_t *lt = (h6_listener_t*)l;
	h6_listener_ops *ops = (h6_listener_ops*)u;

	lt->port = 0;
	lt->host = 0;
	lt->sock = -1;
	lt->event = NULL;
	lt->ops = NULL;
	lt->user_data = NULL;

	if (ops->init)
	{
		err = (*ops->init)(lt);
		if (err)
		{
			return err;
		}
	}

	lt->ops = ops;
	return 0;
}

static void
h6_listener_fin(listener_t *l)
{
	h6_listener_t *lt = (h6_listener_t*)l;
    
    if (lt->ops)
    {
        (*lt->ops->fin)(lt);
    }
    
    return;
}

static int32_t
h6_listener_set_port(listener_t *l, uint16_t port)
{
	h6_listener_t *lt;
	h6_ev_t *ev;
	int32_t sock;

	sock = unix_sock_bind(L4_TCP, 0, htons(port), FORCE_BIND);
	if (sock < 0)
	{
		return sock;
	}

	unix_sock_tcp_listen(sock);

	ev = h6_ev_new(sizeof(*ev), sock, EV_READ);
	if (!ev)
	{
		unix_sock_close(sock);
		return -ENOMEM;
	}

	h6_ev_set_callback(ev, h6_listener_on_event, l, h6_listener_on_ev_fin);
	h6_ev_set_timeout((h6_ev_t*)ev, 3*1000);

	unix_sock_set_flags(sock, O_NONBLOCK);

	lt = (h6_listener_t*)l;
    
	lt->port = port;
	lt->host = 0;
	lt->sock = sock;
	lt->event = ev;
	lt->user_data = NULL;

	return 0;
}

static int32_t
h6_listener_run(listener_t *l)
{
	h6_listener_t *lt;
	h6_server *server;

	lt = (h6_listener_t *)l;
	if (!lt->event)
	{
		return -EINVAL;
	}

	server = (h6_server*)listener_get_owner((listener_t *)lt);
	h6_sched_add(server->sched, lt->event, 1);

	return 0;
}

static client_t*
h6_listener_generate_client(listener_t *l, void *parm)
{
	client_t *c = NULL;
	h6_listener_t *lt = (h6_listener_t *)l;

	if (lt->ops && lt->ops->new_cli)
	{
		c = (*lt->ops->new_cli)(lt, (int32_t)parm);
	}

	return c;
}

static listener_ops l_ops =
{
	.init		= h6_listener_init,
	.fin		= h6_listener_fin,
	.set_port	= h6_listener_set_port,
	.run		= h6_listener_run,
	.new_cli	= h6_listener_generate_client
};

listener_t *
alloc_h6_listener(h6_listener_ops *ops)
{
    return listener_alloc(sizeof(h6_listener_t), &l_ops, ops);
}
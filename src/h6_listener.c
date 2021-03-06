#include <errno.h>
#include <unistd.h>
#include "unix_sock.h"
#include "h6_listener.h"
#include "h6_basic_server.h"
#include "h6_def.h"
#include "trace.h"

static h6_bool_t
h6_listener_on_event(h6_ev_t *ev, int revents, void *user_data)
{
	h6_listener_t *lt;
	int32_t sock;
	client_t *cli;
	h6_svr_t *server;

	lt = (h6_listener_t *)h6_ev_u(ev);

	if (revents & EV_TIMER)
	{
		TRACE_DEBUG("=== LISTEN WAITING... ===\r\n");
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

		server = (h6_svr_t *)listener_get_owner((listener_t *)lt);
		h6_server_add_client(server, cli);

		TRACE_TRACE(
			"Add new client, sock:'%d'.", sock
		);

		client_attach(cli, server->sched);

		TRACE_TRACE("Generate attached client, sock:'%d'", sock);
	}
	
	return H6_TRUE;
}

static void
h6_listener_on_ev_fin(h6_ev_t *ev)
{
	h6_listener_t *lt;

	TRACE_ENTER_FUNCTION;
    
	lt = (h6_listener_t*)h6_ev_u(ev);
	if (lt && lt->ops)
	{
    	lt->ops->fin(lt);
	}
    
	TRACE_EXIT_FUNCTION;
}

static int32_t
h6_listener_init(listener_t *l, void *u)
{
	int32_t err;
	h6_listener_t *lt = (h6_listener_t*)l;
	h6_listener_ops *ops = (h6_listener_ops*)u;

    TRACE_ENTER_FUNCTION;
    
	lt->port = 0;
	lt->host = 0;
	lt->sock = -1;
	lt->event = NULL;
	lt->ops = NULL;
	lt->user_data = NULL;

	if (ops && ops->init)
	{
		err = (*ops->init)(lt);
		if (err)
		{
            TRACE_EXIT_FUNCTION;
            
			return err;
		}
	}

	lt->ops = ops;

    TRACE_EXIT_FUNCTION;
    
	return 0;
}

static void
h6_listener_fin_this(h6_listener_t *lt)
{
    h6_svr_t *svr;

    TRACE_ENTER_FUNCTION;
   
    if (lt->event)
    {
        svr = (h6_svr_t*)listener_get_owner((listener_t *)lt);        
        h6_sched_remove(svr->sched, lt->event);
        h6_ev_unref(lt->event); // destroy ev
    }

    if (lt->sock >= 0)
    {
        unix_sock_close(lt->sock);
    }

    TRACE_EXIT_FUNCTION;    
}

static void
h6_listener_fin(listener_t *l)
{
	h6_listener_t *lt = (h6_listener_t*)l;

    TRACE_ENTER_FUNCTION;
    
    if (lt->ops)
    {
        (*lt->ops->fin)(lt);
    }

    h6_listener_fin_this(lt);

    TRACE_EXIT_FUNCTION;
    
    return;
}

static int32_t
h6_listener_set_port(listener_t *l, uint16_t port)
{
	h6_listener_t *lt;
	h6_ev_t *ev;
	int32_t sock;
	h6_svr_t *server;

	lt = (h6_listener_t*)l;

    TRACE_ENTER_FUNCTION;
    
    if (lt->sock != -1)
    {
        TRACE_ERROR("Listener(%p) has been set port already, port = %u\r\n",
            lt, lt->port);
        TRACE_EXIT_FUNCTION;
        return -1;
    }
    
	sock = unix_sock_bind(L4_TCP, 0, htons(port), FORCE_BIND);
	if (sock < 0)
	{
        TRACE_ERROR("Listener(%p) failed to bind port(%u).\r\n", lt, port);
        
        TRACE_EXIT_FUNCTION;
		return sock;
	}

	unix_sock_tcp_listen(sock);

	ev = h6_ev_new(sizeof(*ev), sock, EV_READ);
	if (!ev)
	{
        TRACE_ERROR("Listener(%p) failed to allocate ev object.\r\n", lt);       
		unix_sock_close(sock);
        
        TRACE_EXIT_FUNCTION;        
		return -ENOMEM;
	}

	server = (h6_svr_t*)listener_get_owner((listener_t *)lt);
    if (server == NULL || server->sched == NULL)
    {
        TRACE_ERROR("Listener(%p) haven't owner/server or server's scheduler\
            didn't created.\r\n", lt);
        unix_sock_close(sock);

        TRACE_EXIT_FUNCTION;
        return -1;
    }

	h6_ev_set_callback(ev, h6_listener_on_event, l, h6_listener_on_ev_fin);
	h6_ev_set_timeout((h6_ev_t*)ev, 3*1000);
                      
	unix_sock_set_flags(sock, O_NONBLOCK);
    
	lt->port = port;
	lt->host = 0;
	lt->sock = sock;
	lt->event = ev;
	lt->user_data = NULL;

   	h6_sched_add(server->sched, lt->event, 1);        

	TRACE_EXIT_FUNCTION;
    
	return 0;
}

/*
static void
h6_listener_kill(listener_t *l)
{
	h6_svr_t *server;    
	h6_listener_t *lt = (h6_listener_t *)l;

    TRACE_ENTER_FUNCTION;
    
    server = (h6_svr_t*)listener_get_owner((listener_t *)lt);
    if (server && server->sched && lt->event)
    {
        h6_sched_remove(server->sched, lt->event);
        h6_ev_unref(lt->event);
    }

    obj_unref(lt);

    TRACE_EXIT_FUNCTION;
}
*/

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
//	.kill       = h6_listener_kill,	
	.new_cli	= h6_listener_generate_client
};

listener_t *
alloc_h6_listener(h6_listener_ops *ops)
{
    return listener_alloc(sizeof(h6_listener_t), &l_ops, ops);
}



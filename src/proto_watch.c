#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "proto_watch.h"
#include "trace.h"
#include "unix_sock.h"
#include "h6_def.h"

#define PROTO_WATCH_WEIGHT		1
#define MAX_BACKLOG_SIZE		65536


enum
{
	STAT_OK,
	STAT_KILLED
};


static __inline__ int32_t
__proto_watch_killed(proto_watch *w)
{
	return w->state == STAT_KILLED;
}


static __inline__ void
__proto_watch_on_closed(proto_watch *w, void *u)
{
	void (*on_closed)(proto_watch *, void *);

	if (w->ops && w->ops->conn_closed)
	{
		on_closed = w->ops->conn_closed;

		pthread_mutex_unlock(w->lock);
		(*on_closed)(w, u);
		pthread_mutex_lock(w->lock);
	}

	TRACE_TRACE("pw '%p' closed, fd:'%d', u:'%p'", w, w->watch_fd, u);
	close(w->watch_fd);
	w->watch_fd = -1;
}


static __inline__ void
__proto_watch_on_error(proto_watch *w, int32_t err, void *u)
{
	void (*on_error)(proto_watch *, int32_t err, void *);

	if (w->ops && w->ops->conn_error)
	{
		on_error = w->ops->conn_error;

		pthread_mutex_unlock(w->lock);
		(*on_error)(w, err, u);
		pthread_mutex_lock(w->lock);
	}

	TRACE_TRACE("pw '%p' error, fd:'%d', u:'%p'.", w, w->watch_fd, u);
	close(w->watch_fd);
	w->watch_fd = -1;
}


static __inline__ int32_t
proto_watch_write_directly(proto_watch *w, const uint8_t *buffer,
	uint32_t *idx, uint32_t size)
{
	int32_t ret, err;
	uint32_t left;

	if (*idx > size)
	{
		return -EINVAL;
	}

	left = size - *idx;

	while (left > 0)
	{
		ret = send(w->watch_fd, &buffer[*idx], left, MSG_DONTWAIT);
		if (ret == 0)
		{
			return -EAGAIN;
		}
		else if (ret < 0)
		{
			err = -errno;
			if (err != -EINTR)
			{
				return err;
			}

			return -EAGAIN;		/* continue */
		}
		else
		{
			left -= ret;
			 *idx += ret;
		}
  	}

	return 0;
}


static __inline__ int32_t
__proto_watch_write_pending(proto_watch *w)
{
	int32_t err;

	for (;;)
	{
		if (!w->write_buffer)
		{
			if (list_empty(&w->backlog_list))
			{
				h6_ev_remove_events_sync((h6_ev_t*)w, EV_WRITE);
				return 0;
			}

			w->write_buffer = list_entry(w->backlog_list.next, mb_t, list);
			list_del(w->backlog_list.next);
			w->backlog_size -= w->write_buffer->size;
		}

		err = proto_watch_write_directly(w, w->write_buffer->ptr,
			&w->write_buffer->offset, w->write_buffer->size);
		if (err)
		{
			return err;
		}

		BUG_ON(w->write_buffer->offset != w->write_buffer->size);
		free_memblock(w->write_buffer);
		w->write_buffer = NULL;
	}

	return 0;
}


static __inline__ int32_t
__proto_watch_recv(proto_watch *w, msg *m, void *u)
{
	int32_t err = 0;

	if (w->ops && w->ops->proto_msg_recv)
	{
		pthread_mutex_unlock(w->lock);

		TRACE_TRACE("pw '%p' recv msg.", w);
		err = (*w->ops->proto_msg_recv)(w, m, u);
		TRACE_TRACE("pw '%p' process msg ok.", w);

		pthread_mutex_lock(w->lock);
	}

	return err;
}


static __inline__ h6_bool_t
__proto_watch_dispath(proto_watch *w, int32_t revents, void *u)
{
	msg *m;
	int32_t err = -EAGAIN;
	socklen_t len = sizeof(err);

	if (revents & EV_TIMER)
	{
		TRACE_TRACE("__proto_watch_dispath(pw '%p')->timeout!", w);
		goto conn_err;
	}

	if (revents & EV_WRITE)
	{
		if (w->connecting)
		{
			w->connecting = 0;
			if (getsockopt(w->watch_fd, SOL_SOCKET, SO_ERROR, &err, &len))
			{
				TRACE_TRACE("__proto_watch_dispath()->getsockopt() failed.");
				err = -errno;
				goto conn_err;
			}
			else
			{
				if (err)
				{
					TRACE_TRACE("__proto_watch_dispath()->connect() failed.");
					err = -err;
					goto conn_err;
				}
			}
		}
		else
		{
			err = __proto_watch_write_pending(w);
			if (err && err != -EAGAIN)
			{
				goto conn_err;
			}
		}
	}

	if (revents & EV_READ)
	{
__parse_again:
		m = parse_proto_io(w->parser, (void*)w->watch_fd, &err);
		if (!m)
		{
			if (err != -EAGAIN)
			{
				if (err == -ECONNRESET)
				{
					TRACE_TRACE("pw '%p' connction reset by peer!", w);
					goto conn_closed;
				}
				else
				{
					TRACE_TRACE("pw '%p' connection error!", w);
					goto conn_err;
				}
			}
		}
		else
		{
			err = __proto_watch_recv(w, m, u);
			if (err)
			{//@{Note, goes to the same way as @err}
				TRACE_TRACE(
					"__proto_watch_dispath()->(*w->ops->proto_msg_recv)() failed,"
					"err:'%d'.", err);
				goto conn_err;
			}
			goto __parse_again;
		}
	}

	return H6_TRUE;

conn_closed:
	__proto_watch_on_closed(w, u);
	return H6_FALSE;

conn_err:
	__proto_watch_on_error(w, err, u);
	return H6_FALSE;
}


static h6_bool_t
proto_watch_dispath(h6_ev_t *ev, int32_t revents, void *u)
{
	h6_bool_t ret = H6_FALSE;
	proto_watch *w = (proto_watch*)ev;

	pthread_mutex_lock(w->lock);
	if (!__proto_watch_killed(w))
	{
		ret = __proto_watch_dispath(w, revents, u);
	}
	pthread_mutex_unlock(w->lock);

	return ret;
}


static __inline__ void
free_backlog_list(struct list_head *head)
{
	struct list_head *l;
	mb_t *mb;

	while (!list_empty(head))
	{
		l = head->next;
		list_del(l);
		mb = list_entry(l, mb_t, list);
		free_memblock(mb);
	}
}


static __inline__ void
proto_watch_fin_self(proto_watch *w)
{
	void *user = h6_ev_u(w);

	if (w->watch_fd > 0)
	{
		//@{Default: so_linger off, close() return immediately,
		//system keep sending itself.}
		close(w->watch_fd);
	}

	if (w->parser)
	{
		if (w->ops->release_proto_parser)
		{
			(*w->ops->release_proto_parser)(w->parser, user);
		}
	}

    pthread_mutex_destroy(w->lock);
    free(w->lock);

	if (!list_empty(&w->backlog_list))
	{
		free_backlog_list(&w->backlog_list);
	}

	if (w->write_buffer)
	{
		free_memblock(w->write_buffer);
	}

	TRACE_TRACE("pw '%p' finalized, u:'%p'.", w, user);
}


static void
proto_watch_on_finalize(h6_ev_t *ev)
{
	proto_watch *w = (proto_watch*)ev;

	proto_watch_fin_self(w);

	if (w->fin)
	{
		(*w->fin)(h6_ev_u(ev));
	}
}


proto_watch *
proto_watch_new(void *io, int32_t timeout, proto_watch_ops *ops, void *u,
	proto_watch_on_fin on_finalize)
{
	proto_watch *pw;
	h6_ev_t *ev;
	proto_parser *p = NULL;

	if (!io || !ops)
	{
		TRACE_WARNING("proto_watch_new()->(io/ops == NULL).");
		return NULL;
	}

	ev = h6_ev_new(sizeof(proto_watch), (int32_t)io, EV_READ);
	if (!ev)
	{
		TRACE_WARNING("proto_watch_new()->j_event_new() failed.");
		return NULL;
	}

	pw = (proto_watch*)ev;
	pw->watch_fd = -1;

	if (ops->create_proto_parser)
	{
		p = (*ops->create_proto_parser)(u);
		if (!p)
		{
			TRACE_WARNING("proto_watch_new()->(*create_proto_parser)() failed.");
			h6_ev_unref(ev);
			return NULL;
		}
	}
	else
	{
	    TRACE_ERROR("Bug happened in function: %s, line:%u\r\n", __FUNCTION__, 
            __LINE__);
	}

	pw->watch_fd = (int32_t)io;
	pw->parser = p;
	pw->fin = on_finalize;
	pw->state = STAT_OK;
	pw->window_size = 0;
	pw->connecting = 0;
	pw->sched = NULL;
	pw->write_buffer = NULL;
	INIT_LIST_HEAD(&pw->backlog_list);
	pw->backlog_size = 0;

    pw->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(pw->lock, NULL);

	pw->ops = ops;
	BUG_ON(unix_sock_set_flags(pw->watch_fd, O_NONBLOCK));

	h6_ev_set_callback(ev, proto_watch_dispath, u,
		proto_watch_on_finalize);

	if (timeout)
	{
		h6_ev_set_timeout((h6_ev_t*)pw, timeout);
	}

	return (proto_watch*)ev;
}


proto_watch *
proto_watch_ref(proto_watch *w)
{
	h6_ev_ref((h6_ev_t*)w);
	return w;
}


void
proto_watch_unref(proto_watch *w)
{
	h6_ev_unref((h6_ev_t*)w);
}


static __inline__ void
__proto_watch_kill(proto_watch *w)
{
	if (!__proto_watch_killed(w))
	{
		w->state = STAT_KILLED;
		if (w->sched)
		{
			h6_sched_remove(w->sched, (h6_ev_t*)w);
			w->sched = NULL;
		}
	}
}


static __inline__ void
proto_watch_kill(proto_watch *w)
{
	pthread_mutex_lock(w->lock);
	__proto_watch_kill(w);
	pthread_mutex_unlock(w->lock);
}


void proto_watch_kill_unref(proto_watch *w)
{
	proto_watch_kill(w);
	proto_watch_unref(w);
}


int32_t
proto_watch_set_window(proto_watch *w, uint32_t win_size)
{
	if (w && win_size > 0)
	{
		w->window_size = win_size;
		return 0;
	}

	return -EINVAL;
}


static __inline__ int32_t
__proto_watch_attach(proto_watch *w, void *sched)
{
	if (__proto_watch_killed(w) || !sched)
		return -EINVAL;

	if (w->sched)
		return -EEXIST;

	w->sched = sched;
	h6_sched_add(sched, (h6_ev_t*)w, PROTO_WATCH_WEIGHT);
	return 0;
}


int32_t proto_watch_attach(proto_watch *w, void *sched)
{
	int32_t err;

	if (!w || !sched || __proto_watch_killed(w))
		return -EINVAL;

	pthread_mutex_lock(w->lock);
	err = __proto_watch_attach(w, sched);
	pthread_mutex_unlock(w->lock);

	return err;
}


static __inline__ void
proto_watch_adjust_window(proto_watch *w, uint32_t seq)
{
	mb_t *mb;
	struct list_head *l, *next;

	if (w->window_size)
	{
		l = w->backlog_list.next;
		while (l != &w->backlog_list)
		{
			mb = list_entry(l, mb_t, list);
			if (!MEMBLOCK_DROPABLE(mb))
			{
				l = l->next;
				continue;
			}
			if (seq - MEMBLOCK_GET_SEQ(mb) <= w->window_size)
				break;
			next = l->next;
			list_del(l);
			w->backlog_size -= mb->size;
			free_memblock(mb);
			l = next;
		}
	}
}


int32_t
proto_watch_write_mb(proto_watch *w, mb_t *mb, uint32_t flags)
{
	int32_t err, pending = 0;

	pthread_mutex_lock(w->lock);

	if (__proto_watch_killed(w))
	{
		free_memblock(mb);
		err = -EKILLED;
		goto write_done;
	}

	if (!w->connecting && !w->write_buffer && list_empty(&w->backlog_list))
	{
		err = proto_watch_write_directly(w, mb->ptr, &mb->offset, mb->size);
		if (err != -EAGAIN)
		{
			free_memblock(mb);
			goto write_done;
		}

		w->write_buffer = mb;
		pending = 1;
		err = 0;
		goto write_done;
	}

	proto_watch_adjust_window(w, MEMBLOCK_GET_SEQ(mb));
	if (w->backlog_size + mb->size > MAX_BACKLOG_SIZE)
	{
		free_memblock(mb);
		err = -EAGAIN;
		goto write_done;
	}

	pending = 1;
	list_add_tail(&mb->list, &w->backlog_list);
	w->backlog_size += mb->size;
	err = 0;

write_done:
	pthread_mutex_unlock(w->lock);

	if (pending)
	{
		h6_ev_add_events((h6_ev_t*)w, EV_WRITE);
	}

	return err;
}


int32_t
proto_watch_write(proto_watch *w, msg *m, uint32_t seq, uint32_t flags)
{//@{Destroy msg certainly}
	mb_t *mb = msg_to_mb(m);

	if (mb)
	{
		MEMBLOCK_SET_SEQ(mb, seq);
		return proto_watch_write_mb(w, mb, flags);
	}

	msg_unref(m);
	return -EINVAL;
}


static __inline__ int32_t
__proto_watch_set_dst(proto_watch *w, uint8_t *ip, int32_t port)
{
	int32_t err;

	err = unix_sock_connect(w->watch_fd, ip, port);
	if (!err)
	{
		return 0;
	}

	if (err == -EINPROGRESS)
	{
		w->connecting = 1;
		h6_ev_add_events((h6_ev_t*)w, EV_WRITE);
		return 0;
	}

	return err;
}


int32_t
proto_watch_writeable(proto_watch *w, uint32_t size)
{//@{writeable: return 0}
	int32_t err = -EAGAIN;

	if (size > MAX_BACKLOG_SIZE)
	{//@{size too large, writeable, maybe drop}
		return 0;
	}

	pthread_mutex_lock(w->lock);
	if (w->backlog_size + size <= MAX_BACKLOG_SIZE)
		err = 0;
	pthread_mutex_unlock(w->lock);

	return err;
}


int32_t
proto_watch_set_dst(proto_watch *w, uint8_t *ip, int32_t port)
{
	int32_t err = -EKILLED;

	pthread_mutex_lock(w->lock);
	if (!__proto_watch_killed(w))
	{
		err = __proto_watch_set_dst(w, ip, port);
	}
	pthread_mutex_unlock(w->lock);

	return err;
}


//:~ End



#ifndef __H6_EV_H__
#define __H6_EV_H__

#include <ev.h>
#include "h6_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define h6_ev_fd(ev) (((h6_ev_t*)ev)->ev_fd)
#define h6_ev_u(ev)  (((h6_ev_t*)ev)->user_data)

typedef struct _h6_ev        h6_ev_t;
typedef struct _h6_ev_opt_t  h6_ev_opt;
typedef struct _h6_ev_loop_t h6_ev_loop;

struct _h6_ev
{
	size_t  ev_size;		/* for memory releasing */
	
    int     ev_fd;
	struct  ev_io io;

	int     timeout;	    /* milli-secs */    
	struct  ev_timer timer;

	int     events;
	int     flags;		    /* internal flags, identify whether h6_ev_t
                             * had been added into loop
                             */
	void    *user_data;

    h6_ev_opt    *opt;
};

typedef h6_bool_t (*h6_ev_dispath)(h6_ev_t *ev, int revents, void *user_data);
typedef void    (*h6_ev_ondestroy)(h6_ev_t *ev);	/* destroy notify */

struct _h6_ev_opt_t
{
	h6_ev_loop      *loop;
	h6_ev_dispath   dispath;
	h6_ev_ondestroy notify;
};

h6_ev_loop *h6_ev_loop_new( void );
void h6_ev_loop_run(h6_ev_loop *loop);
void h6_ev_loop_quit(h6_ev_loop *loop);
h6_bool_t h6_ev_loop_attach(h6_ev_loop *loop, h6_ev_t *event);
void h6_ev_loop_remove(h6_ev_loop* loop, h6_ev_t *event);	/* remove from it's loop */
void h6_ev_loop_free(h6_ev_loop *loop);


h6_ev_t *h6_ev_new(size_t size, int fd, int events);

void h6_ev_set_callback(h6_ev_t *event, h6_ev_dispath dispath,
    void *user_data, h6_ev_ondestroy notify);
    
void h6_ev_set_timeout(h6_ev_t *event, int timeout);

void h6_ev_remove_events(h6_ev_t *event, int events);
void h6_ev_add_events(h6_ev_t *event, int events);

void h6_ev_add_events_sync(h6_ev_t *event, int events);
void h6_ev_mod_timer_sync(h6_ev_t *event, ev_tstamp after);	/* triggered after 'after' secs */
void h6_ev_remove_events_sync(h6_ev_t *event, int events);

ev_tstamp h6_ev_time_now_sync(h6_ev_t *event);



#ifdef __cplusplus
}
#endif

#endif
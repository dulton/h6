AUTOMAKE_OPTIONS=foreign
bin_PROGRAMS = h6
h6_SOURCES = obj.c \
             queue.c \
             unix_sock.c \
             trace.c \
             msg.c \
             mem_block.c \
             h6_ev.c \
             h6_sched.c \
             listener.c \
             h6_listener.c \
             client.c \
             main.c \
             h6_basic_sever.c \
    	     h6_local_proxy_server.c
			 
#client_set.c \
#             network_client.c proto_parser.c proto_watch.c \
#             h6_client.c h6_server.c h6_relay_set.c

h6_CFLAGS=-g -fPIC -Wall 
h6_CXXFLAGS=-g -fPIC -Wall -D_GNU_SOURCE

h6_LDFLAGS = -static
h6_LDADD = -lpthread -lev

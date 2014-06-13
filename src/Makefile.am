AUTOMAKE_OPTIONS=foreign
bin_PROGRAMS = h6
h6_SOURCES = main.c client.c obj.c listener.c queue.c unix_sock.c trace.c client_set.c \
             mem_block.c msg.c network_client.c proto_parser.c proto_watch.c trace.c \
             h6_ev.c h6_sched.c h6_client.c h6_server.c h6_relay_set.c h6_listener.c 

h6_CFLAGS=-g -fPIC -Wall 
h6_CXXFLAGS=-g -fPIC -Wall -D_GNU_SOURCE

h6_LDADD = libpthread.a libev.a

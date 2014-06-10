AUTOMAKE_OPTIONS=foreign
lib_LIBRARIES=libh6.a
libh6_a_SOURCES=client.c obj.c listener.c queue.c unix_sock.c trace.c client_set.c \
                h6_ev.c h6_sched.c h6_server.c h6_relay_set.c h6_listener.c
libh6_a_LIBADD=
libh6_a_LIBFLAGS=-shared
libh6_a_CFLAGS=-g -fPIC -Wall 
libh6_a_CXXFLAGS=-g -fPIC -Wall -D_GNU_SOURCE

libh6includedir=$(prefix)/include/h6
libh6include_HEADERS=../include/*.h
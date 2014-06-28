#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trace.h"
#include "h6_local_proxy_server.h"

#define H6_VERSION  "1.0.0"
#define H6_DESCRIPTION "a p2p proxy or relay server"

typedef enum
{
    UNKNOWN,
    LOCAL_PROXY,
    SERVER_RELAY,
    PEER_PROXY
} svr_mode_t;

void
version(char *name)
{
    fprintf(stdout, "%s v%s - %s\r\n", name, H6_VERSION, H6_DESCRIPTION);
    fprintf(stdout, "compiled at %s %s\r\n\r\n", __DATE__, __TIME__);
}

void
usage(char *name)
{
    fprintf(stdout, "Usage: %s [options]\r\n", name);
    fprintf(stdout, "\t--version \tdisplay version information.\r\n");
    fprintf(stdout, "\t--mode=[options] It can be running in following mode:\r\n");
    fprintf(stdout, "\t\t local-proxy: \r\n");
    fprintf(stdout, "\t\t server-replay: \r\n");        
    fprintf(stdout, "\t\t peer-proxy: \r\n");
    fprintf(stdout, "\r\n");
}

void start_local_proxy()
{
    h6_local_proxy_svr_t *svr;
    char ch;
    
    svr = h6_local_proxy_server_alloc(
            sizeof(h6_local_proxy_svr_t)
            ,NULL
            ,NULL
            ,__FUNCTION__);
    
    if (svr)
    {
        h6_local_proxy_server_run(svr, 2);
        h6_local_proxy_server_bind_port(svr, 1025);

        TRACE_TRACE("press 'X' key to quit ...\r\n");
        do 
        {
            ch = getchar();
        } while (ch != 'X');

        h6_local_proxy_server_remove_port(svr, 1025);
        h6_local_proxy_server_kill_unref(svr);
    }

    
    return;
}

int 
main(int argc, char *argv[])
{
    int i;
    svr_mode_t mode = UNKNOWN;
    
    if (argc < 2)
    {
        usage(argv[0]);
        exit(-1);
    }

    for (i = 1; i < argc; i++)
    {
	if (strcmp(argv[i] , "--version") == 0)
	{
            version(argv[0]);
	    exit(0);
	}
        else if (strcmp(argv[i], "--mode=local-proxy") == 0)
        {
            mode = LOCAL_PROXY;
        }
        else if (strcmp(argv[i], "--mode=server-relay") == 0)
        {
            mode = SERVER_RELAY;
        }
        else if (strcmp(argv[i], "--mode=peer-proxy") == 0)
        {
            mode = PEER_PROXY;
        }
        else
        {
            fprintf(stderr, "Unknow parameters: %s\r\n", argv[i]);
            usage(argv[0]);
        }
    }

    async_trace_init();
    trace_adjust(TRACE_DETAIL_LEVEL);
    
    switch(mode)
    {
    case LOCAL_PROXY:
        start_local_proxy();        
        break;
        
    default:
        break;
    }

    async_trace_destroy();
    
    exit(0);
}

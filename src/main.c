#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h6_server.h"

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

    exit(0);
}

/*
 * callhome.c - Part of amprd version 2.1
 *
 * Author: Marius Petrescu, YO2LOJ, <marius@yo2loj.ro>
 *
 * ---------------------------------------
 *  To my son Marcel Petrescu (2005-2017)
 * ---------------------------------------
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "callhome.h"
#include "commons.h"

static int hcount = 0;

void callhome(Tunnel *tunnel, int alive)
{
    int sd;
    struct sockaddr_in sin;

    char *shutdown = "shutdown";

    if (tunnel->home_data[0] && (0 == hcount))
    {

	if ((sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
	    perror("call home socket():");
	    return;
	}

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = tunnel->prefix.sin_addr.s_addr;
	sin.sin_port = 0;

	if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)))
	{
	    perror("call home bind():");
	    close(sd);
	    return;
	}

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = CALLHOME_ADDRESS;
	sin.sin_port = htons(CALLHOME_PORT);

	if (debug) fprintf(stderr, "Calling home for tunnel %s on port %d: %s\n", tunnel->name, CALLHOME_PORT, tunnel->home_data);

	if (alive)
	{
	    sendto(sd, tunnel->home_data, strlen(tunnel->home_data), 0, (struct sockaddr *)&sin, sizeof(sin));
	}
	else
	{
	    sendto(sd, shutdown, strlen(shutdown), 0, (struct sockaddr *)&sin, sizeof(sin));
	}

	close(sd);

	hcount = CALLHOME_CYCLE * 2;
    }
    else
    {
	if (hcount) hcount--;
    }
}

/*
 * rip.c - part of ripd version 2.1
 *
 * Author: Marius Petrescu, YO2LOJ, <marius@yo2loj.ro>
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <asm/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/route.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <time.h>
#include <ctype.h>

#include "list.h"
#include "tunnel.h"
#include "cache.h"
#include "amprd.h"
#include "route.h"
#include "rip.h"
#include "commons.h"


#define RIP_HDR_LEN		4
#define RIP_ENTRY_LEN		(2+2+4*4)
#define RIP_CMD_REQUEST		1
#define RIP_CMD_RESPONSE	2
#define RIP_AUTH_PASSWD		2
#define RIP_AF_INET		2


#define PERROR(s)				fprintf(stderr, "%s: %s\n", (s), strerror(errno))
#define rip_pcmd(cmd)				((cmd==1)?("Request"):((cmd==2)?("Response"):("Unknown")))


typedef struct __attribute__ ((__packed__))
{
    uint8_t command;
    uint8_t version;
    uint16_t zeros;
} rip_header;


typedef struct __attribute__ ((__packed__))
{
    uint16_t af;
    uint16_t rtag;
    uint32_t address;
    uint32_t mask;
    uint32_t nexthop;
    uint32_t metric;
} rip_entry;

typedef struct __attribute__ ((__packed__))
{
    uint16_t auth;
    uint16_t type;
    char pass[16];
} rip_auth;


int rip_process_auth(int tunid, char *buf, int len)
{
	rip_auth *auth = (rip_auth *)buf;
	Tunnel *tunnel = tunnels + tunid;

	if (auth->auth == 0xFFFF)
	{
	    if (ntohs(auth->type) != RIP_AUTH_PASSWD)
	    {
		if (debug) fprintf(stderr, "Unsupported authentication type %d.\n", ntohs(auth->type));
		return -1;
	    }

	    if ((TRUE == tunnel->rip_recv) && (0 != tunnel->rip_passwd[0]))
	    {
		if (strncmp(auth->pass, tunnel->rip_passwd, 16) == 0)
		{
		    if (debug) fprintf(stderr, "Password validated for %s.\n", tunnel->name);
		    return 1; /* password validated */
		}
	    }

	    if (debug) fprintf(stderr, "Password found in first RIPv2 entry but not configured for %s.\n", tunnel->name);
	
	    if (ntohs(auth->type) == RIP_AUTH_PASSWD)
	    {
		if (debug) fprintf(stderr, "Simple password: %s\n", auth->pass);
	    }
	
	    return -1; /* invalid password */
	}
	else
	{
	    if ((TRUE == tunnel->rip_recv) && (0 == tunnel->rip_passwd[0]))
	    {
		if (debug) fprintf(stderr, "Accept without password for %s.\n", tunnel->name);
		return 0; /* accept without password */
	    }
	
	    if (debug) fprintf(stderr, "No password found in first RIPv2 entry but expected by %s.\n", tunnel->name);
	    return -1;
	}
}

void rip_process_entry(int tunid, char *buf)
{
	rip_entry *rip = (rip_entry *)buf;
	route_entry *p;
	Tunnel * tunnel = tunnels + tunid;
	
	if (ntohs(rip->af) != RIP_AF_INET)
	{
		if (debug && verbose) fprintf(stderr, "Unsupported address family %d.\n", ntohs(rip->af));
		return;
	}

	unsigned int mask = 1;
	unsigned int netmask = 0;
	int i;

	for (i=0; i<32; i++)
	{
	    if (rip->mask & mask)
	    {
		netmask++;
	    }
	    mask <<= 1;
	}

	if (debug && verbose)
	{
		fprintf(stderr, "Entry: address %s/%d ", inet_ntoa(*(struct in_addr *)&rip->address), netmask);
		fprintf(stderr, "nexthop %s ", inet_ntoa(*(struct in_addr *)&rip->nexthop));
		fprintf(stderr, "metric %d ", ntohl(rip->metric));
		fprintf(stderr, "for %s", (tunnels + tunid)->name);
	}

	/* validate and update the route */

	if (check_ignore(tunid, rip->nexthop))
	{
	    if (debug && verbose) fprintf(stderr, " - ignored\n");
	    return;
	}

	/* remove if unreachable and in list */

	if (ntohl(rip->metric) > 14)
	{
		if (debug && verbose) fprintf(stderr, " - unreacheable");
		if ((p = list_find(tunid, rip->address, netmask)) != NULL)
		{
			list_remove(p);
			if (tunnel->rip_set) route_func(tunid, ROUTE_DEL, rip->address, netmask, 0);
			updated[tunid] = TRUE;
			if (debug && verbose) fprintf(stderr, ", removed from list");
		}
		if (debug && verbose) fprintf(stderr, ".\n");
		return;
	}

	if (debug && verbose) fprintf(stderr, "\n");

	if (rip->address == inet_addr("44.0.0.1"))
	{
	    if (rip->nexthop != general_ampr_gw)
	    {
		if (debug && verbose) fprintf(stderr, "Updating default gw: %s", inet_ntoa(*(struct in_addr *)&rip->nexthop));
		general_ampr_gw = rip->nexthop;
	    }
	}

	list_update(tunid, rip->address, netmask, rip->nexthop);
	route_update(tunid, rip->address, netmask, rip->nexthop);
}


int rip_process_message(char *buf, int len)
{
	rip_header *hdr;
	char *bp;
	Tunnel *tunnel;
	
	int res, i, l;

	if (len < RIP_HDR_LEN + RIP_ENTRY_LEN)
	{
		if (debug) fprintf(stderr, "RIP packet to short: %d bytes", len);
		return -1;
	}
	if (len > RIP_HDR_LEN + RIP_ENTRY_LEN * 25)
	{
		if (debug) fprintf(stderr, "RIP packet to long: %d bytes", len);
		return -1;
	}
	if ((len - RIP_HDR_LEN)%RIP_ENTRY_LEN != 0)
	{
		if (debug) fprintf(stderr, "RIP invalid packet length: %d bytes", len);
		return -1;
	}

	/* packet seems plausible, process header */

	hdr = (rip_header *)buf;

	if (debug) fprintf(stderr, "RIP len %d header version %d, Command %d (%s)\n", len, hdr->version, hdr->command, rip_pcmd(hdr->command));

	if (hdr->command != RIP_CMD_RESPONSE)
	{
		if (debug) fprintf(stderr, "Ignored non-response packet\n");
		return -1;
	}

	if (hdr->version != 2)
	{
		if (debug) fprintf(stderr, "Ignored RIP version %d packet (only accept version 2).\n", hdr->version);
		return -1;
	}

	if (hdr->zeros)
	{
		if (debug) fprintf(stderr, "Ignored packet: zero bytes are not zero.\n");
		return -1;
	}

	/* header is valid, process content */

	buf += RIP_HDR_LEN;
	len -= RIP_HDR_LEN;

	/* check password */

	for (i=0; i<numtunnels; i++)
	{
	
	    tunnel = tunnels + i;

	    if (TRUE != tunnel->rip_recv)
		continue;

	    bp = buf;
	    l = len;

	    if ((res = rip_process_auth(i, bp, l)) < 0)
	    {
		/* error */
		continue;
	    }
	
	    if (1 == res)  /* auth ok */
	    {
	        bp += RIP_ENTRY_LEN;
	        l -= RIP_ENTRY_LEN;
	    }

	    /* simple auth ok if needed or not used */

	    if (l <= 0)
	    {
		if (debug) fprintf(stderr, "No routing entries in this packet.\n");
		return -1;
	    }

	    /* we have some entries */

	    if (debug) fprintf(stderr, "Processing RIPv2 packet, %d entries for %s ", l/RIP_ENTRY_LEN, (tunnels + i)->name);
	    if (debug && verbose) fprintf(stderr, "\n");

	    while (l >= RIP_ENTRY_LEN)
	    {
		rip_process_entry(i, bp);
		bp += RIP_ENTRY_LEN;
		l -= RIP_ENTRY_LEN;
	    }

	    if (debug) fprintf(stderr, "(total %d/%d entries).\n", list_count(i), list_count_all());
	}

	/* schedule a route expire check in 30 sec - we do this only if we have route reception */
	/* else we will keep the routes because there are no updates sources available!         */
	
	route_event = TRUE;
	alarm(30);

	return 0;
}



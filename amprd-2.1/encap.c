/*
 * encap.c - Part of amprd version 1.5
 *
 * Author: Marius Petrescu, YO2LOJ, <marius@yo2loj.ro>
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>


#include "encap.h"
#include "tunnel.h"
#include "amprd.h"
#include "commons.h"


char *ipv4_ntoa_encap(route_entry *r)
{
    static char buf[INET_ADDRSTRLEN];
    char *p;
    unsigned int lip = ntohl(r->address);
    sprintf(buf, "%d.%d", (lip & 0xff000000) >> 24, (lip & 0x00ff0000) >> 16);
    if ((((lip & 0x0000ff00) >> 8) != 0) || ((lip & 0x000000ff) != 0))
    {
	p = &buf[strlen(buf)];
	sprintf(p, ".%d", (lip & 0x0000ff00) >> 8);
	if ((lip & 0x000000ff) != 0)
	{
	    p = &buf[strlen(buf)];
	    sprintf(p, ".%d", lip & 0x000000ff);
	}
    }
    return buf;
}


void save_encap(uint16_t tunid)
{
	FILE *efd;
	time_t clock;

	char fname[FILENAME_MAX];

	Tunnel *tunnel = tunnels + tunid;
	route_entry *rt=list_head(tunid);

	snprintf(fname, FILENAME_MAX,"%s/%s.txt", RTFILE_PATH,  tunnel->name);

	efd = fopen(fname, "w+");
	if (NULL == efd)
	{
		if (debug) fprintf(stderr, "Can not open encap file for writing: %s\n", fname);
		return;
	}

	clock = time(NULL);

	fprintf(efd, "#\n");
	fprintf(efd, "# %s.txt encap file - saved by amprd (UTC) %s", tunnel->name, asctime(gmtime(&clock)));
	fprintf(efd, "#\n");

	while (rt->next)
	{
	    rt = rt->next;

	    fprintf(efd, "route addprivate %s", ipv4_ntoa_encap(rt));
	    fprintf(efd, "/%d encap ", rt->netmask);
	    fprintf(efd, "%s\n", inet_ntoa(*(struct in_addr *)&rt->nexthop));
	}
	
	fprintf(efd, "# --EOF--\n");

	fclose(efd);
}

void load_encap(uint16_t tunid)
{
	int count = 0;
	FILE *efd;
	char buffer[255];
	char *p;
	uint32_t b1, b2, b3, b4, nr;
	uint32_t ipaddr;
	uint32_t netmask;
	uint32_t nexthop;

	route_entry *hd=list_head(tunid);
	route_entry *entry;

	Tunnel *tunnel = tunnels + tunid;

	char fname[FILENAME_MAX];
	snprintf(fname, FILENAME_MAX,"%s/%s.txt", RTFILE_PATH, tunnel->name);

	efd = fopen(fname, "r");
	if (NULL == efd)
	{
		if (debug) fprintf(stderr, "Can not open encap file for reading: %s\n", fname);
		return;
	}

	while (fgets(buffer, 255, efd) != NULL)
	{
		if ((buffer[0]!='#') && ((p = strstr(buffer, "addprivate ")) != NULL))
		{
		    p = &p[strlen("addprivate ")];
		    b1 = b2 = b3 = b4 = 0;
		    netmask = 0;
		    ipaddr = 0;
		    nr =sscanf(p, "%d.%d.%d.%d", &b1, &b2, &b3, &b4);
		    if (nr < 2) continue;
		    ipaddr = (b1 << 24) | (b2 << 16);
		    if (nr > 2) ipaddr |= b3 << 8;
		    if (nr > 3) ipaddr |= b4;
		    p = strstr(p, "/"); p = &p[1];
		    if (sscanf(p, "%d", &netmask) != 1) continue;
		    p = strstr(p, "encap ");
		    if (p == NULL) continue;
		    p = &p[strlen("encap ")];
		    nr =sscanf(p, "%d.%d.%d.%d", &b1, &b2, &b3, &b4);
		    if (nr < 4) continue;
		    nexthop = (b1 << 24) | (b2 << 16) | b3 << 8 | b4;

		    /* check ignore list */
		    if (TRUE == check_ignore(tunid, htonl(nexthop)))
		    {
			continue;
		    }

		    /* prevent double entries */
		    if (NULL != list_find(tunid, htonl(ipaddr), netmask))
		    {
			continue;
		    }

		    entry = malloc(sizeof(route_entry));
		    if (NULL == entry)
		    {
			fprintf(stderr, "malloc() failed.\n");
			exit(-1);
		    }
		
		    bzero(entry, sizeof(route_entry));
		    if (hd->next)
		    {
			hd->next->prev = entry;
			entry->next = hd->next;
		    }
		    hd->next = entry;
		    entry->prev = hd;
		
		    entry->address = htonl(ipaddr);
		    entry->netmask = netmask;
		    entry->nexthop = htonl(nexthop);
		    entry->timestamp = 1; /* expire at first update */

		    count++;
		}
	}

	if (debug) fprintf(stderr, "Loaded %d entries from %s\n", count, fname);

	fclose(efd);
}


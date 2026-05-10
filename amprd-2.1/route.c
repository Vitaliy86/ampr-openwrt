/*
 * route.c - part of amprd version 1.5
 *
 * Author: Marius Petrescu, YO2LOJ
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
#include <linux/fib_rules.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <time.h>
#include <ctype.h>


#include "route.h"
#include "tunnel.h"
#include "list.h"
#include "commons.h"



#define PERROR(s)				fprintf(stderr, "%s: %s\n", (s), strerror(errno))
#define NLMSG_TAIL(nmsg)			((struct rtattr *) (((void *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))
#define addattr32(n, maxlen, type, data) 	addattr_len(n, maxlen, type, &data, 4)
#define rta_addattr32(rta, maxlen, type, data)	rta_addattr_len(rta, maxlen, type, &data, 4)


int seq = 0;

uint32_t gwdev;
uint32_t defgw;


int addattr_len(struct nlmsghdr *n, int maxlen, int type, const void *data, int alen)
{
    int len = RTA_LENGTH(alen);
    struct rtattr *rta;

    if ((NLMSG_ALIGN(n->nlmsg_len) + len) > maxlen)
    {
	if (debug) fprintf(stderr, "Max allowed length exceeded during NLMSG assembly.\n");
	return -1;
    }
    rta = NLMSG_TAIL(n);
    rta->rta_type = type;
    rta->rta_len = len;
    memcpy(RTA_DATA(rta), data, len);
    n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + len;
    return 0;
}

int rta_addattr_len(struct rtattr *rta, int maxlen, int type, const void *data, int alen)
{
    struct rtattr *subrta;
    int len = RTA_LENGTH(alen);
    if ((RTA_ALIGN(rta->rta_len) + RTA_ALIGN(len)) > maxlen)
    {
	if (debug) fprintf(stderr, "Max allowed length exceeded during sub-RTA assembly.\n");
	return -1;
    }

    subrta = (struct rtattr *)(((void *)rta) + RTA_ALIGN(rta->rta_len));
    subrta->rta_type = type;
    subrta->rta_len = len;
    memcpy(RTA_DATA(subrta), data, alen);
    rta->rta_len = NLMSG_ALIGN(rta->rta_len) + RTA_ALIGN(len);
    return 0;
}

#ifdef NL_DEBUG
void nl_debug(void *msg, int len)
{
    struct rtattr *rtattr;
    struct nlmsghdr *rh;
    struct rtmsg *rm;
    int i;
    unsigned char *c;

    if (debug && verbose)
    {
	for (rh = (struct nlmsghdr *)msg; NLMSG_OK(rh, len); rh = NLMSG_NEXT(rh, len))
	{
	
	    if (NLMSG_ERROR == rh->nlmsg_type)
	    {
		fprintf(stderr, "NLMSG: error\n");
	    }
	    else if (NLMSG_DONE == rh->nlmsg_type)
	    {
		fprintf(stderr, "NLMSG: done\n");
	    }
	    else
	    {
		if ((RTM_NEWROUTE != rh->nlmsg_type) && (RTM_DELROUTE != rh->nlmsg_type) && (RTM_GETROUTE != rh->nlmsg_type) &&
		    (RTM_NEWRULE != rh->nlmsg_type) && (RTM_DELRULE != rh->nlmsg_type) && (RTM_GETRULE != rh->nlmsg_type))
		{
		    fprintf(stderr, "NLMSG: %d\n", rh->nlmsg_type);
		
		    for (i=0; i<((struct nlmsghdr *)msg)->nlmsg_len; i++)
		    {
			c = (unsigned char *)&msg;
			fprintf(stderr, "%u ", c[i]);
		    }
		    fprintf(stderr, "\n");
		}
		else
		{
		    if (RTM_NEWLINK == rh->nlmsg_type)
		    {
			c = (unsigned char *)"request new link/link info (16)";
		    }
		    else if (RTM_NEWROUTE == rh->nlmsg_type)
		    {
			c = (unsigned char *)"request new route/route info (24)";
		    }
		    else if (RTM_DELROUTE == rh->nlmsg_type)
		    {
			c = (unsigned char *)"delete route (25)";
		    }
		    else if (RTM_GETROUTE == rh->nlmsg_type)
		    {
			c = (unsigned char *)"get route (26)";
		    }
		    else if (RTM_NEWRULE == rh->nlmsg_type)
		    {
			c = (unsigned char *)"request new rule/rule info (32)";
		    }
		    else if (RTM_DELRULE == rh->nlmsg_type)
		    {
			c = (unsigned char *)"delete rule (33)";
		    }
		    else if (RTM_GETRULE == rh->nlmsg_type)
		    {
			c = (unsigned char *)"get rule (34)";
		    }
		    else
		    {
			c = (unsigned char *)"";
		    }
		    fprintf(stderr, "NLMSG: %s\n", c);
		    rm = NLMSG_DATA(rh);
		    for (rtattr = (struct rtattr *)RTM_RTA(rm); RTA_OK(rtattr, len); rtattr = RTA_NEXT(rtattr, len))
		    {
			fprintf(stderr, "RTA type: %d (%d bytes): ", rtattr->rta_type, rtattr->rta_len);
			for(i=0; i<(rtattr->rta_len - sizeof(struct rtattr)); i++)
			{
			    c = (unsigned char *)RTA_DATA(rtattr);
			    fprintf(stderr, "%u ", c[i]);
			}
			fprintf(stderr, "\n");
		    }
		}
	    }
	}
    }
}
#endif

uint32_t route_func(uint16_t tunid, rt_actions action, uint32_t address, uint32_t netmask, uint32_t nexthop)
{

    int nlsd;
    int len;

    char nlrxbuf[4096];
    char mxbuf[256];

    struct {
	struct nlmsghdr hdr;
	struct rtmsg    rtm;
	char buf[1024];
    } req;

    struct rtattr *mxrta = (void *)mxbuf;

    struct sockaddr_nl sa;
    struct rtattr *rtattr;
    struct nlmsghdr *rh;
    struct rtmsg *rm;

    uint32_t result = 0;
    uint32_t window = 840;

    Tunnel *tunnel = tunnels + tunid;

    mxrta->rta_type = RTA_METRICS;
    mxrta->rta_len = RTA_LENGTH(0);

    memset(&req, 0, sizeof(req));

    memset(&sa, 0, sizeof(struct sockaddr_nl));
    sa.nl_family = AF_NETLINK;
    sa.nl_pid = getpid();
    sa.nl_groups = 0;

    req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.hdr.nlmsg_flags = NLM_F_REQUEST;
    req.hdr.nlmsg_seq = ++seq;
    req.hdr.nlmsg_pid = getpid();
    req.rtm.rtm_family = AF_INET;
    req.rtm.rtm_dst_len = netmask;
    req.rtm.rtm_protocol = RTPROT_AMPR;
    req.rtm.rtm_table = RT_TABLE_MAIN;

    if (ROUTE_DEL == action)
    {
        req.rtm.rtm_scope = RT_SCOPE_NOWHERE;
        req.rtm.rtm_type = RTN_UNICAST;
        req.hdr.nlmsg_type = RTM_DELROUTE;
        req.hdr.nlmsg_flags |= NLM_F_CREATE;
        result = 1;
    }
    else if (ROUTE_ADD == action)
    {
	req.rtm.rtm_type = RTN_UNICAST;
	req.hdr.nlmsg_type = RTM_NEWROUTE;
	req.hdr.nlmsg_flags |= NLM_F_CREATE;
	result = 1;
    }
    else
    {
	req.hdr.nlmsg_type = RTM_GETROUTE;
	req.rtm.rtm_scope = RT_SCOPE_UNIVERSE;
    }

    addattr32(&req.hdr, sizeof(req), RTA_DST, address);

    if (ROUTE_ADD == action)
    {
	if (nexthop == 0)
	{
	    addattr32(&req.hdr, sizeof(req), RTA_OIF, tunnel->index); /* dev */
	    rta_addattr32(mxrta, sizeof(mxbuf), RTAX_WINDOW, window);
	    addattr_len(&req.hdr, sizeof(req), RTA_METRICS, RTA_DATA(mxrta), RTA_PAYLOAD(mxrta));
	}
	else
	{
	    addattr32(&req.hdr, sizeof(req), RTA_GATEWAY, defgw); /* gateway */
	    addattr32(&req.hdr, sizeof(req), RTA_OIF, gwdev); /* dev */
	}
    }

    if ((nlsd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
    {
	if (debug) fprintf(stderr, "Can not open netlink socket.\n");
	return 0;
    }

    if (bind(nlsd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        if (debug) fprintf(stderr, "Can not bind to netlink socket.\n");
	return 0;
    }
#ifdef NL_DEBUG
    if (debug && verbose) fprintf(stderr, "NL sending request.\n");
    nl_debug(&req, req.hdr.nlmsg_len);
#endif
    if (send(nlsd, &req, req.hdr.nlmsg_len, 0) < 0)
    {
	if (debug) fprintf(stderr, "Can not talk to rtnetlink.\n");
	return 0;
    }

    if ((len = recv(nlsd, nlrxbuf, sizeof(nlrxbuf), MSG_DONTWAIT|MSG_PEEK)) > 0)
    {
#ifdef NLDEBUG
	if (debug && verbose) fprintf(stderr, "NL response received.\n");
	nl_debug(nlrxbuf, len);
#endif
	if (ROUTE_GET == action)
	{
	    /* parse response for ROUTE_GETDEV */
	    for (rh = (struct nlmsghdr *)nlrxbuf; NLMSG_OK(rh, len); rh = NLMSG_NEXT(rh, len))
	    {
	        if (rh->nlmsg_type == 24) /* route info resp */
	        {
		    rm = NLMSG_DATA(rh);
		
		    for (rtattr = (struct rtattr *)RTM_RTA(rm); RTA_OK(rtattr, len); rtattr = RTA_NEXT(rtattr, len))
		    {
			if (RTA_GATEWAY == rtattr->rta_type)
			{
			    result = *((uint32_t *)RTA_DATA(rtattr));
			}
		    }
		}
		else if (NLMSG_ERROR == rh->nlmsg_type)
		{
		    result = 0;
		}
	    }
	}

	if (ROUTE_GETDEV == action)
	{
	    /* parse response for ROUTE_GETDEV */
	    for (rh = (struct nlmsghdr *)nlrxbuf; NLMSG_OK(rh, len); rh = NLMSG_NEXT(rh, len))
	    {
	        if (rh->nlmsg_type == 24) /* route info resp */
	        {
		    rm = NLMSG_DATA(rh);
		
		    for (rtattr = (struct rtattr *)RTM_RTA(rm); RTA_OK(rtattr, len); rtattr = RTA_NEXT(rtattr, len))
		    {
			if (RTA_OIF == rtattr->rta_type)
			{
			    result = *((uint32_t *)RTA_DATA(rtattr));
			}
		    }
		}
		else if (NLMSG_ERROR == rh->nlmsg_type)
		{
		    result = 0;
		}
	    }
	}
    }

    close(nlsd);
    return result;
}

void route_update(uint16_t tunid, uint32_t address, uint32_t netmask, uint32_t nexthop)
{
    Tunnel *tunnel = tunnels + tunid;
    struct in_addr addr;

    if (tunnel->rip_set)
    {
	if (route_func(tunid, ROUTE_GETDEV, address, netmask, 0) != tunnel->index)
	{
	    route_func(tunid, ROUTE_DEL, address, netmask, 0); /* fails if route does not exist - no problem */


	    if (route_func(tunid, ROUTE_ADD, address, netmask, 0) == 0)
	    {
		if (debug)
		{
		    addr.s_addr = address;
		    fprintf(stderr, "Failed to set route %s/%d on dev %s.\n", inet_ntoa(addr), netmask, tunnel->name);
		}
	    }
	}
    }

    if (44 == (nexthop & 0x000000FF))
    {
	route_func(tunid, ROUTE_DEL, nexthop, 32, 0); /* fails if route does not exist - no problem */
	addr.s_addr = address;
	if (debug) fprintf(stderr, "Set host route to %s on dev %d.\n", inet_ntoa(addr), gwdev);
	if (route_func(tunid, ROUTE_ADD, nexthop, 32, defgw) == 0) /* add host route */
	{
	    if (debug)
	    {
		addr.s_addr = address;
		fprintf(stderr, "Failed to set host route to %s on dev %d.\n", inet_ntoa(addr), gwdev);
	    }
	}
    }
}

void route_set_all(void)
{
    int i;
    Tunnel *tunnel;
    route_entry *p;

    for (i=0; i<numtunnels; i++)
    {
	tunnel = tunnels + i;
	
	if (tunnel->rip_recv)
	{
	    if (debug) fprintf(stderr, "Setting all routes on %s.\n", tunnel->name);
	
	    if (tunnel->rip_recv)
	    {
		p = list_head(i);
		while (p->next)
		{
		    p = p->next;
		    route_update(i, p->address, p->netmask, p->nexthop);
		}
	    }
	}
    }
}

void route_clear_all(void)
{
    int i;
    Tunnel *tunnel;
    route_entry *p;

    for (i=0; i<numtunnels; i++)
    {
	tunnel = tunnels + i;
	
	if (debug) fprintf(stderr, "Clearing all routes on %s.\n", tunnel->name);
	
	if (tunnel->rip_recv)
	{
	    p = list_head(i);
	    while (p->next)
	    {
		p = p->next;
		route_func(i, ROUTE_DEL, p->address, p->netmask, 0);
		if (44 == (p->nexthop & 0x000000FF))
		{
		    route_func(i, ROUTE_DEL, p->nexthop, 32, 0);
		}
	    }
	}
    }
}


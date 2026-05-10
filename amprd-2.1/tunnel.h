/*
 *    tunnel.h - part of ripd version 1.5
 *
 *    Author: Marius Petrescu, YO2LOJ
 */

#ifndef AMPRD_TUNNEL_H
#define AMPRD_TUNNEL_H

#include <netinet/in.h>
#include <net/if.h>
#include <sys/types.h>
#include <linux/if_ether.h>
#include "commons.h"

typedef struct __attribute__((__packed__))
{
   int                  fd;
   struct ifreq		ifr;
   char 		name[IFNAMSIZ + 1];
   int			index;
   unsigned char	hwa[ETH_ALEN];
   /* tunnel data */
   struct sockaddr_in	prefix;
   struct sockaddr_in	mask;
   /* rip data */
   int			rip_recv;
   int			rip_save;
   int			rip_set;
   char			rip_passwd[17];
   /* ignore */
   char			rip_ignore[257];
   uint32_t		rip_ignore_ip[MAXIGNORE];
   /* rip forward */
   int			rip_fws;
   char			rip_forward[IFNAMSIZ+1];
   char			home_data[MAXHOMEDATA + 1];
} Tunnel;

extern int numtunnels;
extern Tunnel *tunnels;

int init_tun(char *config_file);

#endif

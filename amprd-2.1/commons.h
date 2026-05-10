/*
 * commons.h - Part of amprd version 2.1
 *
 * Author: Marius Petrescu, YO2LOJ, <marius@yo2loj.ro>
 *
 */

#ifndef AMPRD_COMMONS_H
#define AMPR_COMMONS_H

#include <stdint.h>

#define FALSE	0
#define TRUE	(!FALSE)

#define RTFILE_PATH		"/var/lib/amprd"
#define CONF_FILE		"/etc/amprd.conf"
#define PID_FILE		"/var/run/amprd.pid"

#define ROUTE_EXPIRE_TIME	900	/* route expire time */
#define GENERAL_AMPR_GW		(inet_addr("169.228.34.84"))

#define CALLHOME_ADDRESS	(inet_addr("44.182.21.1"))	/* call home destinetion */
#define CALLHOME_PORT		59002				/* call home UDP port */

#define RTCACHE_SIZE		32	/* route cache size */
#define RTCACHE_EXPTIME		60	/* cache expire time */
#define RTPROT_AMPR		44	/* PROTO to be used on set routes for routing diferentiation purposes */
#define MYIPSIZE		16	/* max number of local interface IPs */
#define MAXIGNORE		16	/* max number of hosts in the interface ignore list */
#define MAXHOMEDATA		21	/* max length of call home data */

//#define NL_DEBUG

extern int debug;
extern int verbose;

extern int *updated;
extern char *passwd;

extern uint32_t general_ampr_gw;

#endif

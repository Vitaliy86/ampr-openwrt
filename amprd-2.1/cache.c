/*
 * cache.c - Part of amprd version 1.5
 *
 * Author: Marius Petrescu, YO2LOJ, <marius@yo2loj.ro>
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "tunnel.h"
#include "list.h"
#include "commons.h"

struct cache_ent {
    uint32_t address;
    uint32_t gateway;
    uint32_t timestamp;
    uint16_t tunnel;
};


struct cache_ent cache[RTCACHE_SIZE];


uint32_t cache_find_tunnel(uint16_t tunid, uint32_t address)
{
    int i = RTCACHE_SIZE;
    uint32_t found = 0;
    uint32_t tm = time(NULL);

    while (i--)
    {
	if ((cache[i].timestamp + RTCACHE_EXPTIME) < tm)
	    cache[i].timestamp = 0;

	if ((tunid == cache[i].tunnel) && (address == cache[i].address))
	{
	    found = cache[i].gateway;
	}
    }
    return found;
}

uint32_t cache_find(uint32_t address)
{
    int i = RTCACHE_SIZE;
    uint32_t found = 0;
    uint32_t tm = time(NULL);

    while (i--)
    {
	if ((cache[i].timestamp + RTCACHE_EXPTIME) < tm)
	    cache[i].timestamp = 0;

	if (address == cache[i].address)
	{
	    found = cache[i].gateway;
	}
    }
    
    return found;
}

void cache_add(uint16_t tunid, uint32_t address, uint32_t gateway)
{
    int i = RTCACHE_SIZE;
    uint32_t otime = 0;
    int oldest = -1;
    uint32_t tm = time(NULL);

    while (i--)
    {
	if ((cache[i].timestamp + RTCACHE_EXPTIME) < tm)
	    cache[i].timestamp = 0;

	if (!cache[i].timestamp)
	{
	    oldest = i;
	    break;
	}

	if (cache[i].timestamp > otime)
	{
	    otime = cache[i].timestamp;
	    oldest = i;
	}
    }
    cache[oldest].address = address;
    cache[oldest].gateway = gateway;
    cache[oldest].tunnel = tunid;
    cache[oldest].timestamp = time(NULL);
}

void cache_flush(uint16_t tunid)
{
    int i = RTCACHE_SIZE;
    while (i--)
    {
	if (tunid == cache[i].tunnel)
	    cache[i].timestamp = 0;
    }
}

uint32_t cache_lookup(uint16_t tunid, uint32_t address)
{
    uint32_t gateway = 0;
    Tunnel *tunnel = tunnels + tunid;
    route_entry *rt = NULL;

    if (tunnel->rip_recv)
    {
	gateway = cache_find_tunnel(tunid, address);
    }
    else
    {
	gateway = cache_find(address);
    }

    if (!gateway)
    {
        if (tunnel->rip_recv)
	{
	    rt = list_get(tunid, address);
	}
	else
	{
	    rt = list_get_all(address);
	}
    }
    else
    {
	return gateway;
    }

    if (rt)
    {
	gateway = rt->nexthop;
    }
    else
    {
	gateway = general_ampr_gw;
    }

    cache_add(tunid, address, gateway);

    return gateway;
}

void cache_init(void)
{
    bzero(cache, sizeof(cache));
}

/*
 * list.c - Part of amprd version 1.5
 *
 * Author: Marius Petrescu, YO2LOJ, <marius@yo2loj.ro>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "list.h"
#include "tunnel.h"
#include "commons.h"

static route_entry *head;

void list_add(uint16_t tunid, uint32_t address, uint32_t netmask, uint32_t nexthop)
{
	route_entry *n;

	n = malloc(sizeof(route_entry));
	if (NULL == n)
	{
		fprintf(stderr, "Error: malloc failed.\n");
		exit(-1);
	}

	memset((char *)n, 0, sizeof(n));

	n->address = address;
	n->netmask = netmask;
	n->nexthop = nexthop;
	n->timestamp = time(NULL);
	if (head[tunid].next)
	{
		n->next = head[tunid].next;
		n->next->prev = n;
	}
	n->prev = &head[tunid];
	head[tunid].next = n;

}

int list_count(uint16_t tunid)
{
	route_entry *p = &head[tunid];
	int count = 0;
	while(p->next)
	{
	    p=p->next;
	    count++;
	}
	return count;
}

int list_count_all(void)
{
    int i;
    int count = 0;
    for(i=0; i<numtunnels; i++)
    {
	count += list_count(i);
    }
    return count;
}

route_entry *list_find(uint16_t tunid, uint32_t address, uint32_t netmask)
{
	route_entry *p = &head[tunid];

	while (p->next)
	{
		p = p->next;
		if ((p->address == address) && (p->netmask == netmask))
		{
			return p;
		}
	}
	return NULL;
}

void list_update(uint16_t tunid, uint32_t address, uint32_t netmask, uint32_t nexthop)
{
	route_entry *p;
	if ((p = list_find(tunid, address, netmask)) != NULL)
	{
		/* update timestamp and nexthop */
		if (p->nexthop != nexthop)
		{
		    p->nexthop = nexthop;
		    updated[tunid] = TRUE;
		}
		p->timestamp = time(NULL);
	}
	else
	{
		list_add(tunid, address, netmask, nexthop);
		updated[tunid] = TRUE;
	}
}

void list_remove(route_entry *entry)
{
	if (entry->next)
	{
	    entry->prev->next = entry->next;
	    entry->next->prev = entry->prev;
	}
	else
	{
	    entry->prev->next = NULL;
	}
	free(entry);
}

void list_clear(uint16_t tunid)
{
    while (head[tunid].next)
    {
        list_remove(head[tunid].next);
    }
}

void list_clear_all(void)
{
    int i;
    for(i=0; i<numtunnels; i++)
    {
	list_clear(i);
    }
}

route_entry *list_get(uint16_t tunid, uint32_t address)
{
	route_entry *p = &head[tunid];
	
	int mask = 0;
	route_entry *found = NULL;
	int bmask, i;
	
	while (p->next) 
	{
	    p=p->next;
	
	    bmask = 0;
	    for (i = 0; i < p->netmask; i++)
		bmask |= (0x80000000 >> i);
	    bmask = htonl(bmask);
	
	    if ((p->address & bmask) == (address & bmask))
	    {
		if (mask < p->netmask)
		{
		    mask = p->netmask;
		    found = p;
		}
	    }
	
	    if (32 == mask)
		break;
	}
	return found;
}

route_entry *list_get_all(uint32_t address)
{
	route_entry *p;
	int mask = 0;
	route_entry *found = NULL;
	int bmask, i, tunid;

	for(tunid = 0; tunid<numtunnels; tunid++)
	{
		p = &head[tunid];
		
		while (p->next)
		{
		    p=p->next;
	
		    bmask = 0;
		    for (i = 0; i < p->netmask; i++)
			bmask |= (1 << i);
	
		    if ((p->address & bmask) == (address & bmask))
		    {
			if (mask < p->netmask)
			{
			    mask = p->netmask;
			    found = p;
			}
		    }
		
		    if (32 == mask)
			break;
		}
	}
	return found;
}

route_entry *list_head(uint16_t tunid)
{
	return &head[tunid];
}


void list_init(void)
{
	head = malloc(sizeof(route_entry) * numtunnels);
	if (NULL == head)
	{
		fprintf(stderr, "Error: malloc failed.\n");
		exit(-1);
	}

	memset((char *)head, 0, sizeof(route_entry) * numtunnels);
}



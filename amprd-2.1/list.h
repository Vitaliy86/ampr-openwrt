/*
 * list.h - Part of amprd version 1.5
 *
 * Author: Marius Petrescu, YO2LOJ, <marius@yo2loj.ro>
 *
 */

#ifndef AMPRD_LIST_H
#define AMPRD_LIST_H

#include <stdint.h>

typedef struct s_route_entry
{
    struct s_route_entry *next;
    struct s_route_entry *prev;
    uint32_t address;
    uint32_t netmask;
    uint32_t nexthop;
    uint32_t timestamp;
} route_entry;


void list_init(void);
void list_add(uint16_t tunid, uint32_t address, uint32_t netmask, uint32_t nexthop);
int list_count(uint16_t tunid);
int list_count_all(void);
route_entry *list_find(uint16_t tunid, uint32_t address, uint32_t netmask);
void list_update(uint16_t tunid, uint32_t address, uint32_t netmask, uint32_t nexthop);
void list_remove(route_entry *entry);
void list_clear(uint16_t tunid);
void list_clear_all(void);
route_entry *list_get(uint16_t tunid, uint32_t address);
route_entry *list_get_all(uint32_t address);
route_entry *list_head(uint16_t tunid);

#endif

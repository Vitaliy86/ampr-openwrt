/*
 * route.h - part of amprd version 1.5
 *
 * Author: Marius Petrescu, YO2LOJ
 *
 */

#ifndef AMPRD_ROUTE_H
#define AMPRD_ROUTE_H

#include <stdint.h>

typedef enum
{
    ROUTE_ADD,
    ROUTE_DEL,
    ROUTE_GET,
    ROUTE_GETDEV
} rt_actions;

extern uint32_t gwdev;
extern uint32_t defgw;

uint32_t route_func(uint16_t tunid, rt_actions action, uint32_t address, uint32_t netmask, uint32_t nexthop);

void route_update(uint16_t tunid, uint32_t address, uint32_t netmask, uint32_t nexthop);

void route_set_all(void);
void route_clear_all(void);

#endif

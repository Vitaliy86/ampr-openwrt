/*
 * cache.h - Part of amprd version 1.5
 *
 * Author: Marius Petrescu, YO2LOJ, <marius@yo2loj.ro>
 *
 */

#ifndef AMPRD_LOOKUP_H
#define AMPRD_LOOKUP_H

#include <stdint.h>

void cache_init(void);
uint32_t cache_lookup(uint16_t tunid, uint32_t address);
void cache_flush(uint16_t tunid);

#endif

/*
 * encap.h - Part of amprd version 1.5
 *
 * Author: Marius Petrescu, YO2LOJ, <marius@yo2loj.ro>
 *
 */

#ifndef AMPRD_ENCAP_H
#define AMPRD_ENCAP_H

#include <stdint.h>
#include "list.h"

char *ipv4_ntoa_encap(route_entry *r);
void save_encap(uint16_t tunid);
void load_encap(uint16_t tunid);

#endif

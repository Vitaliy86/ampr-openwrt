/*
 * callhome.h - Part of amprd version 2.1
 *
 * Author: Marius Petrescu, YO2LOJ, <marius@yo2loj.ro>
 *
 */

#ifndef AMPRD_CALLHOME_H
#define AMPRD_CALLHOME_H

#include <stdint.h>
#include "tunnel.h"

#define CALLHOME_CYCLE	 5 /* minutes */

void callhome(Tunnel *tunnel, int alive);

#endif

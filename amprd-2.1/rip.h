/*
 * rip.c - part of ripd version 2.1
 *
 * Author: Marius Petrescu, YO2LOJ, <marius@yo2loj.ro>
 *
 */

#ifndef AMPRD_RIP_H
#define AMPRD_RIP_H

extern int route_event;
int rip_process_message(char *buf, int len);

#endif

#ifndef NETWORK_H
#define NETWORK_H

#include "event.h"

typedef struct {
    double base_delay;   // e.g., 0.01 s
    double jitter;       // e.g., +/- 0.01 s
} Network;

void network_init(Network* n, double base_delay, double jitter);
double network_rand_delay(Network* n);

/* schedule delivery through the network (adds delay, enqueues recv event) */
void network_schedule_delivery(Network* n, double now, EventType recv_event,
                               int src, int dst, void *data);

#endif

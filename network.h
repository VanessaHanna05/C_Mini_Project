#ifndef NETWORK_H
#define NETWORK_H

#include "event.h"

typedef struct {
    double base_delay;   // e.g., 0.01 s
    double jitter;       // e.g., +/- 0.01 s
} Network;

void network_init(Network* n, double base_delay, double jitter);
double network_rand_delay(Network* n); //returns base_delay plus a symmetric random offset in [−jitter, +jitter], clamped at zero.

/* schedule delivery through the network (adds delay, enqueues recv event) */
//does not send anything immediately. 
//It uses schedule_event(now + delay, recv_event, src, dst, data) to enqueue the corresponding receive side event at the other endpoint.
void network_schedule_delivery(Network* n, double now, EventType recv_event,
                               int src, int dst, void *data);

#endif

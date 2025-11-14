#ifndef NETWORK_H
#define NETWORK_H
#include "event.h"

/*
 * Network models one‑way delay as base_delay ± jitter.
 * Simplicity over realism: there’s no bandwidth/queues; only random latency.
 * This is enough to exercise time ordering, losses, and handshake behavior.
 */

typedef struct Network {
    double base_delay;  // mean propagation/transmission delay
    double jitter;      // max deviation around the mean (uniform)
    double sum_delay;   // running total for stats (avg delay)
    int    count_delay; // how many deliveries contributed to stats
} Network;

void   network_init(Network* n, double base_delay, double jitter);
double network_rand_delay(Network* n);

/*
 * network_schedule_delivery: helper that draws a delay and schedules a receive
 * event at the destination’s handler. Keeping this in Network centralizes the
 * randomness and statistics collection rather than scattering it in senders.
 */
void network_schedule_delivery(Network* n, double now,
                               int src, int dst,
                               void *data, EventDataDtor dtor,
                               EventHandler recv_handler, void *recv_ctx);

#endif

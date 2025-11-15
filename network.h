#ifndef NETWORK_H
#define NETWORK_H

#include "event.h"

/* Simple network model: base_delay ± jitter. */
typedef struct Network {
    double base_delay;   // average one-way delay (seconds)
    double jitter;       // max deviation around the average
    double sum_delay;    // accumulate all delays (for average)
    int    count_delay;  // how many delays we’ve recorded
} Network;

void   network_init(Network* n, double base_delay, double jitter);
double network_rand_delay(Network* n);

/* Schedule a receive event at dst through the network.
 *   now         : current simulation time (g_now)
 *   src, dst    : sender and receiver ids
 *   packet_id   : packet id carried in the event (or -1)
 *   recv_handler: function to call at delivery time
 *
 * It:
 *   - chooses a random delay,
 *   - updates delay statistics,
 *   - inserts an event in the global queue at (now + delay). */
void network_schedule_delivery(Network* n, double now,
                               int src, int dst,
                               int packet_id,
                               EventHandler recv_handler);

#endif

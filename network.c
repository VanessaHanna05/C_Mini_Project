#include <stdlib.h>
#include "network.h"
#include "event.h"

/* Helper to generate uniform random numbers in [0,1]. */
static double frand01(void) {
    return rand() / (double)RAND_MAX;
}

void network_init(Network* n, double base_delay, double jitter) {
    n->base_delay  = base_delay;
    n->jitter      = jitter;
    n->sum_delay   = 0.0;
    n->count_delay = 0;
}

/* Return a random delay around base_delay, clamped to >= 0. */
double network_rand_delay(Network* n) {
    double d = n->base_delay + (2.0 * frand01() - 1.0) * n->jitter;
    if (d < 0.0) d = 0.0;
    return d;
}

/* Draw a delay, record it, and schedule delivery of a packet/event. */
void network_schedule_delivery(Network* n, double now,
                               int src, int dst,
                               int packet_id,
                               EventHandler recv_handler)
{
    double d = network_rand_delay(n);  // random network delay
    n->sum_delay   += d;
    n->count_delay += 1;

    schedule_event(now + d, src, dst, packet_id, recv_handler);
}

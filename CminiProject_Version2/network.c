#include <stdlib.h>
#include "network.h"
#include "heap_priority.h"

/* Small helper to generate uniform randoms in [0,1]. */
static double frand01(void){ return rand()/(double)RAND_MAX; }

void network_init(Network* n, double base_delay, double jitter) {
    n->base_delay = base_delay;
    n->jitter     = jitter;
    n->sum_delay  = 0.0;   // reset stats at sim start
    n->count_delay= 0;
}

/* Draw a delay, update stats, and schedule the receiver’s handler.
 * Implication: timing is applied once per hop (sender->receiver) and is
 * transparent to the endpoints; they just see events arriving later. */
void network_schedule_delivery(Network* n, double now,
                               int src, int dst,
                               void *data, EventDataDtor dtor,
                               EventHandler recv_handler, void *recv_ctx)
{
    double d = network_rand_delay(n);
    n->sum_delay  += d;     // capture for later average
    n->count_delay+= 1;

    schedule_event(now + d, src, dst, data, dtor, recv_handler, recv_ctx);
}

/* Uniform delay around base_delay; clamped at 0 to avoid negative times. */
double network_rand_delay(Network* n) {
    double d = n->base_delay + (2.0*frand01()-1.0)*n->jitter;
    return d < 0.0 ? 0.0 : d;
}

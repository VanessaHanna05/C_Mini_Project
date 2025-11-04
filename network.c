#include <stdlib.h>
#include "network.h"
#include "heap_priority.h"

static double frand01(void) { return rand() / (double)RAND_MAX; }

void network_init(Network* n, double base_delay, double jitter) {
    n->base_delay = base_delay;
    n->jitter = jitter;
}

double network_rand_delay(Network* n) {
    double d = n->base_delay + (2.0 * frand01() - 1.0) * n->jitter; // uniform in [base-jitter, base+jitter]
    if (d < 0.0) d = 0.0;
    return d;
}

void network_schedule_delivery(Network* n, double now, EventType recv_event,
                               int src, int dst, void *data)
{
    double d = network_rand_delay(n);
    schedule_event(now + d, recv_event, src, dst, data);
}

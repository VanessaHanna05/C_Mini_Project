#include <stdlib.h>     // For rand, RAND_MAX
#include "network.h"    // Our Network struct and prototypes
#include "heap_priority.h" // For schedule_event used in network_schedule_delivery

/*
frand01:
- Helper function that returns a random double in the interval [0, 1].
- Uses rand() / (double)RAND_MAX to normalize the integer rand() output.
- Marked static so it's only visible within this file.
*/
static double frand01(void) { 
    return rand() / (double)RAND_MAX; 
}

/*
network_init:
- Initializes the fields of the Network struct pointed to by n.
- base_delay: sets the average round-trip or one-way delay we expect.
- jitter    : sets how much the delay can vary up or down.
*/
void network_init(Network* n, double base_delay, double jitter) {
    n->base_delay = base_delay;  // store the base delay
    n->jitter     = jitter;      // store the jitter amplitude
}

/*
network_rand_delay:
- Uses base_delay and jitter to compute a random delay.

Steps:
1. frand01() returns a value u in [0,1].
2. (2.0 * u - 1.0) transforms [0,1] into [-1,1].
3. Multiply that by jitter to get an offset in [-jitter, +jitter].
4. Add this offset to base_delay.
5. If final delay is negative, clamp it to 0.0 (we don't allow negative times).
*/
double network_rand_delay(Network* n) {
    // 1-4. Compute jittered delay
    double d = n->base_delay + (2.0 * frand01() - 1.0) * n->jitter;

    // 5. Clamp to zero if result went negative
    if (d < 0.0) d = 0.0;

    return d; // final non-negative delay
}

/*
network_schedule_delivery:
- Models the process of sending a packet from src to dst through the network.
- Instead of directly calling the receiver, we:
    * compute a random delivery time: now + network_rand_delay(n),
    * schedule a new event at that future time.
- This lets the discrete-event simulator control when the "receive" happens.

Parameters:
- n          : Network model (used for delay).
- now        : current simulation time (g_now).
- recv_event : type of event that the receiver will get (e.g. EVT_RECV_DATA).
- src, dst   : ids of sender and receiver.
- data       : pointer to payload (e.g., int* packet id).
*/
void network_schedule_delivery(Network* n, double now, EventType recv_event,
                               int src, int dst, void *data)
{
    // First compute a random one-way delay through the network
    double d = network_rand_delay(n);

    // Then schedule the receive event at time "now + d" in the event queue.
    schedule_event(now + d, recv_event, src, dst, data);
}

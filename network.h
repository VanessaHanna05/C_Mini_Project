#ifndef NETWORK_H
#define NETWORK_H
/*
Header guard for Network module.
*/

#include "event.h"
/*
We include event.h because the network operations schedule events of given type.
*/

/*
Network:
- Represents a simple, abstract model of a network link.
- base_delay : the average or fixed delay applied to every packet.
- jitter     : the maximum variation around the base delay.
               Actual delay will be base_delay + random_offset,
               where random_offset is in the range [-jitter, +jitter].
*/
typedef struct {
    double base_delay;   // e.g., 0.01 seconds (10 ms base latency)
    double jitter;       // e.g., +/- 0.01 seconds jitter around base_delay
} Network;

/*
network_init:
- Initializes a Network struct with given parameters.
- This function sets the base delay and jitter so that other functions
  (like network_rand_delay) can use them.
*/
void   network_init(Network* n, double base_delay, double jitter);

/*
network_rand_delay:
- Computes and returns a random delay based on the network parameters.
- It uses:
    delay = base_delay + random_in[-jitter, +jitter]
- If resulting delay is negative, it is clamped to 0 (no negative delays).
*/
double network_rand_delay(Network* n);

/*
network_schedule_delivery:
- Simulates sending a packet through the network by scheduling a future
  receive event for the other side.

Parameters:
- n          : pointer to the Network model, used to compute the delay.
- now        : current simulation time (g_now).
- recv_event : the EventType that will be delivered to the receiver (e.g. EVT_RECV_DATA).
- src        : id of the sending endpoint.
- dst        : id of the receiving endpoint.
- data       : pointer to payload, passed through to the receive event.

Behavior:
- Calls network_rand_delay to get a random delay d.
- Uses schedule_event(now + d, recv_event, src, dst, data) to enqueue
  the receive event at the correct future time, instead of delivering
  anything immediately.
*/
void network_schedule_delivery(Network* n, double now, EventType recv_event,
                               int src, int dst, void *data);

#endif // NETWORK_H

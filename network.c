#include <stdlib.h> 
// Provides rand(), RAND_MAX for generating randomness.
#include <stdio.h>
#include "network.h"  
#include "event.h"
#define PROB_INVALID 0.05  
// 5% probability that a DATA packet's ID will be corrupted to an invalid value

/*
   frand01(): returns a random floating-point number in [0,1].
   rand() returns an integer between 0 and RAND_MAX.
   Dividing by RAND_MAX to make it return a number between 0 and 1.
    */
 double frand01(void) {
    return rand() / (double)RAND_MAX;
}


/* Initializes a Network structure with base delay and jitter.
   base_delay = average one-way latency (seconds)
   jitter = the maximum amount the actual delay can vary above or below base_delay */
void network_init(Network* n, double base_delay, double jitter) {
    n->base_delay  = base_delay;   // Mean delay for every packet
    n->jitter      = jitter;       // Max variation around the mean
    n->sum_delay   = 0.0;          // Accumulates all delays seen (cumulative sum)
    n->count_delay = 0;            // Counts how many packets passed through the network
}


/* 
   Computes one random network delay
      d = base_delay + (2*R - 1) * jitter
   where R = random number in [0,1].
   (2R - 1) generates a random value in [-1, +1].
   Multiplying by jitter gives a random deviation around the base_delay.
   If the random result is negative, put it  0. */
double network_rand_delay(Network* n) {
    double d = n->base_delay + (2.0 * frand01() - 1.0) * n->jitter;

    if (d < 0.0) d = 0.0;  

    return d;
}


/* Schedules when a packet (packet_id) is delivered from src to dst.

   Inputs:
     now          = current simulation time
     src, dst     =  sender (0)/receiver(1) IDs
     packet_id    = which packet is being delivered
     recv_handler = function that should run when the packet arrives
     Compute a random network delay.
     Update total delay 
     Schedule an event at time (now + delay) that calls recv_handler*/
void network_schedule_delivery(Network* n, double now,int src, int dst,int packet_id, EventHandler recv_handler)
{
    double d = network_rand_delay(n);  
    n->sum_delay   += d;
    n->count_delay += 1;
    int final_pkt_id = packet_id;
    if (packet_id >= 0) {
        
        if (frand01() < PROB_INVALID) {
            final_pkt_id = -2;
             printf("[%.3f] Network: CORRUPTED pkt id %d -> %d\n", now, packet_id, final_pkt_id);
        }
    }
    schedule_event(now + d, src, dst, final_pkt_id, recv_handler);
}


#ifndef NETWORK_H
#define NETWORK_H

#include "event.h"

typedef struct Network {
    double base_delay;   
    double jitter;      
    double sum_delay;    
    int    count_delay;  
} Network;

void   network_init(Network* n, double base_delay, double jitter);
double network_rand_delay(Network* n);

void network_schedule_delivery(Network* n, double now, int src, int dst, int packet_id, EventHandler recv_handler);

#endif

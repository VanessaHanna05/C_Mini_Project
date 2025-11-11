#ifndef RECEIVER_H
#define RECEIVER_H

#include "event.h"
#include "network.h"   

typedef struct {
    int id;
    int received_ok; //counter of valid DATA packets
    int invalid_packets; //counter for bad or unexpected things
} Receiver;

void receiver_init(Receiver *r, int id);
void receiver_handle(Receiver *r, Network* net, Event *e);

#endif

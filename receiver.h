#ifndef RECEIVER_H
#define RECEIVER_H

#include "event.h"
#include "network.h"   

typedef struct {
    int id;
    int received_ok;
    int invalid_packets;
} Receiver;

void receiver_init(Receiver *r, int id);
void receiver_handle(Receiver *r, Network* net, Event *e);

#endif

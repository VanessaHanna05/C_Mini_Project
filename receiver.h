#ifndef RECEIVER_H
#define RECEIVER_H

#include "event.h"

#define RCV_MAX_PKTS 10000

typedef struct Receiver {
    int id;
    int received_ok;          
    int unique_ok;           
    int invalid_packets;
    char seen[RCV_MAX_PKTS];  
} Receiver;

void receiver_init(Receiver *r, int id);
void rcv_recv_syn();
void rcv_recv_ack();
void rcv_recv_data(struct Event *e);
void rcv_recv_finish();

#endif

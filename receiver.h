#ifndef RECEIVER_H
#define RECEIVER_H

#include "event.h"

/* Match MAX_PKTS from sender.h (same value!) */
#define RCV_MAX_PKTS 10000

typedef struct Receiver {
    int id;
    int received_ok;          // counts every DATA arrival (including retransmissions)
    int unique_ok;            // counts distinct packet ids seen at least once
    int invalid_packets;
    char seen[RCV_MAX_PKTS];  // seen[i] == 1 if we already counted pkt i
} Receiver;

void receiver_init(Receiver *r, int id);
void rcv_recv_syn(struct Event *e);
void rcv_recv_ack(struct Event *e);
void rcv_recv_data(struct Event *e);
void rcv_recv_finish(struct Event *e);

#endif

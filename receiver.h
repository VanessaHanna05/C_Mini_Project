#ifndef RECEIVER_H
#define RECEIVER_H
#include "event.h"

/* Minimal state: we only count good vs invalid packets for demo purposes. */
typedef struct Receiver {
    int id;               // logical endpoint identifier
    int received_ok;      // how many data packets were accepted
    int invalid_packets;  // how many malformed/unexpected packets we saw
} Receiver;

void receiver_init(Receiver *r, int id);

/* Receiver event handlers called by the scheduler. */
void rcv_recv_syn(void *ctx, struct Event *e);
void rcv_recv_ack(void *ctx, struct Event *e);
void rcv_recv_data(void *ctx, struct Event *e);
void rcv_recv_finish(void *ctx, struct Event *e);

#endif

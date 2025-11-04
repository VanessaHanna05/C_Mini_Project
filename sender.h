#ifndef SENDER_H
#define SENDER_H

#include "event.h"
#include "network.h"

typedef struct {
    int id;
    int sent;                  // number of DATA attempts actually scheduled (valid packets only)
    int lost_events_counter;   // events "sent" but not scheduled (simulate local loss before network)
    int empty_counter;         // empty/non-valid packets that are NOT scheduled to network
    int drop_counter;          // dropped due to DATA ACK timeout (no retransmit)
    double next_send_time;
    double send_interval;      // seconds
    double duration;           // seconds (stop condition)
    int next_pkt_id;

    /* ack tracking for timeouts */
    int max_pkts;
    char *acked;               // acked[pkt_id] => 1 if received
} Sender;

void sender_init(Sender *s, int id, double send_interval_s, double duration_s, int max_pkts);
void sender_handle(Sender *s, Network* net, Event *e);

#endif

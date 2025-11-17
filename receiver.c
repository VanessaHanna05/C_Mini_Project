#include <stdio.h>
#include "receiver.h"
#include "globals.h"
#include "network.h"
#include "sender.h"

extern double g_now;


void receiver_init(Receiver *r, int id) {
    r->id              = id;
    r->received_ok     = 0;
    r->unique_ok       = 0;
    r->invalid_packets = 0;

    for (int i = 0; i < RCV_MAX_PKTS; ++i) {
        r->seen[i] = 0;
    }
}

void rcv_recv_syn(Event *e) {
    (void)e;

    printf("[%.3f] Receiver: RECV SYN -> SEND SYNACK\n", g_now);

    network_schedule_delivery(&g_net, g_now, RECEIVER_ID, SENDER_ID, -1, snd_recv_synack);
}

void rcv_recv_ack(Event *e) {
    (void)e;

    printf("[%.3f] Receiver: RECV ACK (connection established)\n", g_now);
}

void rcv_recv_data(Event *e) {
    int pkt_id = e->packet_id;

    if (pkt_id < 0 || pkt_id >= RCV_MAX_PKTS) {
        g_receiver.invalid_packets++;
        printf("[%.3f] Receiver: RECV DATA with invalid id %d\n", g_now, pkt_id);
        return;
    }

    g_receiver.received_ok++;

    if (!g_receiver.seen[pkt_id]) {
        g_receiver.seen[pkt_id] = 1;
        g_receiver.unique_ok++;
    }

    printf("[%.3f] Receiver: RECV DATA #%d -> SEND ACK\n", g_now, pkt_id);
    network_schedule_delivery(&g_net, g_now, RECEIVER_ID, SENDER_ID, pkt_id, snd_recv_data_ack);
}

void rcv_recv_finish(Event *e) {
    (void)e;

    printf("[%.3f] Receiver: RECV FINISH -> stop simulation\n", g_now);
    g_stop_simulation = 1;
}

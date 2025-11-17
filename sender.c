#include <stdio.h>
#include <stdlib.h>
#include "sender.h"
#include "globals.h"
#include "network.h"
#include "receiver.h"

extern double g_now;
#define RTO 0.05

static double frand01(void) {
    return rand() / (double)RAND_MAX;
}

void sender_init(Sender *s, int id, double send_interval_s, double duration_s) {
    int i;

    s->id            = id;
    s->sent          = 0;
    s->lost_local    = 0;
    s->next_pkt_id   = 0;
    s->send_interval = send_interval_s;
    s->duration      = duration_s;
    s->syn_acked     = 0;
    for (i = 0; i < MAX_PKTS; ++i) {
        s->acked[i] = 0;
    }
    schedule_event(0.0,  s->id, RECEIVER_ID, -1, snd_send_syn);
    schedule_event(RTO,  s->id, s->id,       -1, snd_timeout);
}

void snd_send_syn(Event *e) {
    (void)e;  

    printf("[%.3f] Sender: SEND SYN\n", g_now);
    network_schedule_delivery(&g_net, g_now, SENDER_ID, RECEIVER_ID, -1, rcv_recv_syn);
}
void snd_recv_synack(Event *e) {
    (void)e;  

    g_sender.syn_acked = 1;
    printf("[%.3f] Sender: RECV SYNACK -> SEND ACK, start data\n", g_now);
    network_schedule_delivery(&g_net, g_now, SENDER_ID, RECEIVER_ID, -1, rcv_recv_ack);
    double first_time = g_now + g_sender.send_interval;
    if (first_time <= g_sender.duration) {
        schedule_event(first_time, SENDER_ID, RECEIVER_ID, -1, snd_send_data);
    } else {
        schedule_event(g_now, SENDER_ID, RECEIVER_ID, -1, rcv_recv_finish);
    }
}

void snd_send_data(Event *e) {
    (void)e;  
    if (g_now > g_sender.duration) {
        schedule_event(g_now, SENDER_ID, RECEIVER_ID, -1, rcv_recv_finish);
        return;
    }

    int pkt_id = g_sender.next_pkt_id++;  
    if (frand01() < 0.03) { 
        g_sender.lost_local++;
        printf("[%.3f] Sender: LOCAL DROP of pkt #%d\n", g_now, pkt_id);
    } else {
        printf("[%.3f] Sender: SEND DATA #%d\n", g_now, pkt_id);
        network_schedule_delivery(&g_net, g_now, SENDER_ID, RECEIVER_ID, pkt_id, rcv_recv_data);
        g_sender.sent++;
    }

    schedule_event(g_now + RTO, SENDER_ID, SENDER_ID, pkt_id, snd_timeout);
    double next_time = g_now + g_sender.send_interval;
    if (next_time <= g_sender.duration) {
        schedule_event(next_time, SENDER_ID, RECEIVER_ID, -1, snd_send_data);
    } else {
        schedule_event(g_now, SENDER_ID, RECEIVER_ID, -1, rcv_recv_finish);
    }
}
void snd_recv_data_ack(Event *e) {
    int pkt_id = e->packet_id;

    if (pkt_id >= 0 && pkt_id < MAX_PKTS) {
        g_sender.acked[pkt_id] = 1; 
        printf("[%.3f] Sender: RECV ACK for pkt #%d\n", g_now, pkt_id);
    } else {
        printf("[%.3f] Sender: RECV ACK with invalid pkt id %d\n", g_now, pkt_id);
    }
}
void snd_timeout(Event *e) {
    int pkt_id = e->packet_id;

    if (pkt_id == -1) {
        if (!g_sender.syn_acked) {
            printf("[%.3f] Sender: SYN TIMEOUT -> retransmit SYN\n", g_now);
            schedule_event(g_now,       SENDER_ID, RECEIVER_ID, -1, snd_send_syn);
            schedule_event(g_now + RTO, SENDER_ID, SENDER_ID,   -1, snd_timeout);
        }
    } else {
        if (pkt_id >= 0 && pkt_id < MAX_PKTS && !g_sender.acked[pkt_id]) {
            printf("[%.3f] Sender: TIMEOUT pkt #%d -> retransmit\n", g_now, pkt_id);
            network_schedule_delivery(&g_net, g_now, SENDER_ID, RECEIVER_ID, pkt_id, rcv_recv_data);
            schedule_event(g_now + RTO, SENDER_ID, SENDER_ID, pkt_id, snd_timeout);
        }
    }
}

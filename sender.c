#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sender.h"
#include "heap_priority.h"

extern double g_now; // from main.c

/* probabilities */
#define P_EMPTY_PACKET   0.05  // 5% empty/invalid -> NOW: counted and NOT scheduled
#define P_LOST_EVENT     0.03  // 3% don't schedule event (simulate lost before network)

/* timeouts */
#define SYN_RTO          0.5   // s
#define DATA_ACK_RTO     0.3   // s

static double frand01(void) { return rand() / (double)RAND_MAX; }

static int* new_int_heap(int v) {
    int *p = (int*)malloc(sizeof(int));
    *p = v;
    return p;
}

void sender_init(Sender *s, int id, double send_interval_s, double duration_s, int max_pkts) {
    s->id = id;
    s->sent = 0;
    s->lost_events_counter = 0;
    s->empty_counter = 0;
    s->drop_counter = 0;
    s->next_send_time = 0.0;
    s->send_interval = send_interval_s;
    s->duration = duration_s;
    s->next_pkt_id = 1;
    s->max_pkts = max_pkts;
    s->acked = (char*)calloc(max_pkts + 5, 1);

    /* kick off handshake */
    schedule_event(0.0, EVT_SEND_SYN, s->id, 1, NULL);
    /* add a SYN timeout (track with special pkt_id = 0) */
    schedule_event(SYN_RTO, EVT_TIMEOUT, s->id, s->id, new_int_heap(0));
}

void sender_handle(Sender *s, Network* net, Event *e) {
    switch (e->type) {
    case EVT_SEND_SYN: {
        printf("[%.3f] Sender: SEND SYN\n", g_now);
        /* "send" SYN -> receiver receives SYN */
        network_schedule_delivery(net, g_now, EVT_RECV_SYN, s->id, e->dst, NULL);
        /* (SYN timeout already scheduled at init, will be rearmed on need) */
        break;
    }

    case EVT_RECV_SYNACK: {
        printf("[%.3f] Sender: RECV SYNACK -> SEND ACK, start data\n", g_now);
        /* cancel SYN timeout by marking acked[0]=1 */
        if (s->acked) s->acked[0] = 1;

        /* send final ACK of handshake */
        network_schedule_delivery(net, g_now, EVT_RECV_ACK, s->id, e->dst, NULL);

        /* schedule first DATA send tick after interval, but not beyond duration */
        double first = g_now + s->send_interval;
        if (first <= s->duration)
            schedule_event(first, EVT_SEND_DATA, s->id, e->dst, NULL);
        else
            schedule_event(g_now, EVT_RECV_FINISH, s->id, e->dst, NULL);
        break;
    }

    case EVT_SEND_DATA: {
        if (g_now > s->duration) {
            /* tell receiver we're finished */
            network_schedule_delivery(net, g_now, EVT_RECV_FINISH, s->id, e->dst, NULL);
            break;
        }

        /* maybe "lose" this event before scheduling (simulate local drop) */
        if (frand01() < P_LOST_EVENT) {
            s->lost_events_counter++;
            printf("[%.3f] Sender: LOST before schedule (not enqueued)\n", g_now);
        } else {
            /* decide if this would be an empty/invalid packet */
            if (frand01() < P_EMPTY_PACKET) {
                /* count empty and do NOT schedule anything to the network */
                int temp_id = s->next_pkt_id++; // consume an id for accounting/debug
                s->empty_counter++;
                printf("[%.3f] Sender: EMPTY pkt id=%d (not scheduled)\n", g_now, temp_id);
                /* no timeout, no sent++, nothing goes to the receiver */
            } else {
                /* create a real packet and schedule it */
                Packet* pkt = (Packet*)malloc(sizeof(Packet));
                pkt->id = s->next_pkt_id++;
                pkt->src = s->id; pkt->dst = e->dst;
                pkt->tx_time = g_now;
                pkt->size_bytes = 1000;

                printf("[%.3f] Sender: SEND DATA id=%d\n", g_now, pkt->id);

                /* schedule receiver to get it */
                network_schedule_delivery(net, g_now, EVT_RECV_DATA, s->id, e->dst, pkt);

                /* schedule a DATA ACK timeout (no retransmit; count drop if fires) */
                schedule_event(g_now + DATA_ACK_RTO, EVT_TIMEOUT, s->id, s->id, new_int_heap(pkt->id));

                s->sent++;
            }
        }

        /* schedule next periodic send */
        double nxt = g_now + s->send_interval;
        if (nxt <= s->duration) {
            schedule_event(nxt, EVT_SEND_DATA, s->id, e->dst, NULL);
        } else {
            /* schedule finish notification (so receiver stops and prints done!) */
            network_schedule_delivery(net, g_now, EVT_RECV_FINISH, s->id, e->dst, NULL);
        }
        break;
    }

    case EVT_RECV_DATA_ACK: {
        /* data ack carries pkt_id in data as int* */
        int pkt_id = e->data ? *(int*)e->data : -1;
        if (pkt_id >= 0 && pkt_id <= s->max_pkts) s->acked[pkt_id] = 1;
        printf("[%.3f] Sender: RECV DATA ACK (pkt=%d)\n", g_now, pkt_id);
        if (e->data) free(e->data);
        break;
    }

    case EVT_TIMEOUT: {
        /* timeouts: SYN uses id=0; DATA uses pkt_id > 0 */
        int id = e->data ? *(int*)e->data : -1;
        if (id == 0) {
            /* SYN timeout: retransmit SYN and rearm timeout if handshake not done */
            if (!s->acked[0]) {
                printf("[%.3f] Sender: SYN timeout -> retransmit SYN\n", g_now);
                schedule_event(g_now, EVT_SEND_SYN, s->id, 1, NULL);
                schedule_event(g_now + SYN_RTO, EVT_TIMEOUT, s->id, s->id, new_int_heap(0));
            }
        } else if (id > 0) {
            /* DATA ACK timeout: drop (no retransmit) if not acked */
            if (id <= s->max_pkts && !s->acked[id]) {
                s->drop_counter++;
                printf("[%.3f] Sender: DATA timeout -> DROP pkt=%d\n", g_now, id);
            }
        }
        if (e->data) free(e->data);
        break;
    }

    default:
        break;
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sender.h"         // declares Sender and includes event/network headers
#include "heap_priority.h"  // schedule_event, pop_next_event, etc.

extern double g_now; // from main.c

/* probabilities */
#define P_LOST_EVENT     0.03  // 3% don't schedule event (simulate local loss)

/* timeouts */
#define SYN_RTO          0.5   // s

static double frand01(void) { return rand() / (double)RAND_MAX; }

static int* new_int_heap(int v) {
    int *p = (int*)malloc(sizeof(int));
    *p = v;
    return p;
}

/*
 * NOTE: We keep the original signature so it matches your existing headers/Makefile.
 * 'max_pkts' is unused now because there are no per-packet IDs.
 */
void sender_init(Sender *s, int id, double send_interval_s, double duration_s, int max_pkts) {
    (void)max_pkts; // unused in the simplified model

    s->id = id;
    s->sent = 0;
    s->lost_events_counter = 0;
    /* these may exist in your struct; safe to zero if present */
    s->empty_counter = 0;
    s->drop_counter = 0;

    s->next_send_time = 0.0;
    s->send_interval = send_interval_s;
    s->duration = duration_s;

    /* If your struct has these fields from the old design, neutralize them */
    s->next_pkt_id = 0;
    if (s->acked) s->acked[0] = 0;   // only used to cancel SYN timeout in this simplified version

    /* kick off handshake */
    schedule_event(0.0, EVT_SEND_SYN, s->id, 1, NULL);

    /* add a SYN timeout (track with special id = 0) */
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

        /* cancel SYN timeout by marking acked[0]=1 if available */
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
            /* create a random integer payload and schedule it to the receiver */
            int *payload = (int*)malloc(sizeof(int));
            *payload = rand() % 1000000;  /* random payload */
            printf("[%.3f] Sender: SEND DATA (rand=%d)\n", g_now, *payload);

            network_schedule_delivery(net, g_now, EVT_RECV_DATA, s->id, e->dst, payload);
            s->sent++;
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
        /* In the simplified model, ACK carries no payload; just log it */
        printf("[%.3f] Sender: RECV DATA ACK\n", g_now);
        if (e->data) free(e->data);  /* defensive free if something was attached */
        break;
    }

    case EVT_TIMEOUT: {
        /* Only SYN timeout is kept in this simplified model: id==0 */
        int id = e->data ? *(int*)e->data : -1;
        if (id == 0) {
            /* SYN timeout: retransmit SYN and rearm timeout if handshake not done */
            int handshake_done = (s->acked ? s->acked[0] : 0);
            if (!handshake_done) {
                printf("[%.3f] Sender: SYN timeout -> retransmit SYN\n", g_now);
                schedule_event(g_now, EVT_SEND_SYN, s->id, 1, NULL);
                schedule_event(g_now + SYN_RTO, EVT_TIMEOUT, s->id, s->id, new_int_heap(0));
            }
        }
        if (e->data) free(e->data);
        break;
    }

    default:
        break;
    }
}

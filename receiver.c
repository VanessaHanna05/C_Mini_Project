#include <stdio.h>
#include <stdlib.h>
#include "receiver.h"
#include "heap_priority.h"

extern double g_now;            // from main.c
extern int g_stop_simulation;   // from main.c

static int* new_int_heap(int v) { int *p = (int*)malloc(sizeof(int)); *p = v; return p; }

void receiver_init(Receiver *r, int id) {
    r->id = id;
    r->received_ok = 0;
    r->invalid_packets = 0;
}

void receiver_handle(Receiver *r, Network* net, Event *e) {
    switch (e->type) {
    case EVT_RECV_SYN:
        printf("[%.3f] Receiver: RECV SYN -> SEND SYNACK\n", g_now);
        network_schedule_delivery(net, g_now, EVT_RECV_SYNACK, r->id, e->src, NULL);
        break;

    case EVT_RECV_ACK:
        printf("[%.3f] Receiver: RECV ACK (established)\n", g_now);
        break;

    case EVT_RECV_DATA: {
        Packet* pkt = (Packet*)e->data;
        if (!pkt) { /* shouldn't happen */ break; }
        if (pkt->size_bytes <= 0) {
            r->invalid_packets++;
            printf("[%.3f] Receiver: RECV DATA INVALID (id=%d)\n", g_now, pkt->id);
            /* no ACK for invalid packets */
        } else {
            r->received_ok++;
            printf("[%.3f] Receiver: RECV DATA OK (id=%d)\n", g_now, pkt->id);
            /* send DATA ACK back carrying pkt_id */
            network_schedule_delivery(net, g_now, EVT_RECV_DATA_ACK, r->id, e->src, new_int_heap(pkt->id));
        }
        free(pkt);
        break;
    }

    case EVT_RECV_FINISH:
        printf("done!\n");
        g_stop_simulation = 1; // signal main loop to stop
        break;

    default:
        break;
    }
}

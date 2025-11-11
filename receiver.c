#include <stdio.h>
#include <stdlib.h>
#include "receiver.h"
#include "heap_priority.h"

extern double g_now;
extern int g_stop_simulation;
int* new_int_heap(int v);

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
        int *pkt_id = (int*)e->data;
        if (!pkt_id) break;
        printf("[%.3f] Receiver: RECV DATA #%d -> ACK\n", g_now, *pkt_id);
        r->received_ok++;
        network_schedule_delivery(net, g_now, EVT_RECV_DATA_ACK, r->id, e->src, new_int_heap(*pkt_id));
        free(pkt_id);
        break;
    }

    case EVT_RECV_FINISH:
        printf("done!\n");
        g_stop_simulation = 1;
        break;

    default:
        break;
    }
}

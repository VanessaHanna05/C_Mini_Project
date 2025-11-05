#include <stdio.h>
#include <stdlib.h>

#include "receiver.h"
#include "heap_priority.h"

extern double g_now;            // from main.c
extern int g_stop_simulation;   // from main.c

void receiver_init(Receiver *r, int id) {
    r->id = id;
    r->received_ok = 0;
    r->invalid_packets = 0; // not used in simplified model, but keep the field clean
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
        /* payload is just a random integer allocated by sender */
        int *payload = (int*)e->data;
        if (!payload) break;

        r->received_ok++;
        printf("[%.3f] Receiver: RECV DATA OK from %d (rand=%d) -> ACK\n",
               g_now, e->src, *payload);

        /* ACK back to sender; no per-packet id needed, so payload = NULL */
        network_schedule_delivery(net, g_now, EVT_RECV_DATA_ACK, r->id, e->src, NULL);

        free(payload);  /* receiver owns and frees the payload */
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

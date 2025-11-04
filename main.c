#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "event.h"
#include "heap_priority.h"
#include "network.h"
#include "sender.h"
#include "receiver.h"

/* global state */
double g_now = 0.0;
int g_stop_simulation = 0;

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <send_interval_ms> <duration_s>\n", argv[0]);
        fprintf(stderr, "Example: %s 100 20\n", argv[0]);
        return 1;
    }

    double send_interval_ms = atof(argv[1]);
    double duration_s       = atof(argv[2]);
    if (send_interval_ms <= 0 || duration_s <= 0) {
        fprintf(stderr, "Invalid args: interval_ms and duration_s must be > 0\n");
        return 1;
    }
    double send_interval_s = send_interval_ms / 1000.0;

    srand((unsigned)time(NULL));

    /* components */
    Network net;
    Sender  snd;
    Receiver rcv;

    network_init(&net, /*base_delay*/ 0.01, /*jitter*/ 0.01);
    event_queue_init();
    sender_init(&snd, /*sender_id*/ 0, send_interval_s, duration_s, /*max_pkts*/ 200000);
    receiver_init(&rcv, /*receiver_id*/ 1);

    /* main simulation loop */
    while (!event_queue_empty() && !g_stop_simulation) {
        Event* e = pop_next_event();
        if (!e) break;
        g_now = e->time;

        switch (e->type) {
        /* sender-owned events */
        case EVT_SEND_SYN:
        case EVT_SEND_DATA:
        case EVT_RECV_SYNACK:
        case EVT_RECV_DATA_ACK:
        case EVT_TIMEOUT:
            sender_handle(&snd, &net, e);
            break;

        /* receiver-owned events */
        case EVT_RECV_SYN:
        case EVT_RECV_ACK:
        case EVT_RECV_DATA:
        case EVT_RECV_FINISH:
            receiver_handle(&rcv, &net, e);
            break;

        default:
            /* ignore */
            break;
        }

        /* free event shell (payload freed by handlers) */
        free(e);
    }

    printf("\n=== Simulation finished ===\n");
    printf("Sender: sent_scheduled=%d, lost_events=%d, empty_pkts=%d, data_drops=%d\n",
       snd.sent, snd.lost_events_counter, snd.empty_counter, snd.drop_counter);

    printf("Receiver: ok=%d, invalid=%d\n", rcv.received_ok, rcv.invalid_packets);

    return 0;
}

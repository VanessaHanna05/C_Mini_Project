#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "event.h"
#include "heap_priority.h"
#include "network.h"
#include "sender.h"
#include "receiver.h"
#include "globals.h"

/* Define the global instances declared in globals.h. */
Network  g_net;
Sender   g_sender;
Receiver g_receiver;

int main(int argc, char **argv) {
    double send_interval = 0.05;  // default 50 ms between packets
    double duration      = 1.0;   // default simulation duration
    double base_delay    = 0.2;  // default network base delay
    double jitter        = 0.2;  // default network jitter

    /* Optional command-line arguments:
     *   ./sim
     *   ./sim <send_interval> <duration>
     *   ./sim <send_interval> <duration> <base_delay> <jitter> */
    if (argc == 3 || argc == 5) {
        send_interval = atof(argv[1]);
        duration      = atof(argv[2]);
        if (argc == 5) {
            base_delay = atof(argv[3]);
            jitter     = atof(argv[4]);
        }
    } else if (argc != 1) {
        fprintf(stderr,
            "Usage:\n"
            "  %s                      (use defaults)\n"
            "  %s <send_interval> <duration>\n"
            "  %s <send_interval> <duration> <base_delay> <jitter>\n",
            argv[0], argv[0], argv[0]);
        return 1;
    }

    /* Initialize RNG for random delays and drops. */
    srand((unsigned)time(NULL));

    /* Initialize subsystems. */
    event_queue_init();
    network_init(&g_net, base_delay, jitter);
    receiver_init(&g_receiver, RECEIVER_ID);
    sender_init(&g_sender,  SENDER_ID, send_interval, duration);

    /* Main loop:
     *   - While we are not told to stop and there are events:
     *       * pop next event,
     *       * jump g_now to its time,
     *       * call its handler,
     *       * free the Event. */
    while (!g_stop_simulation && !event_queue_empty()) {
        Event *e = pop_next_event();
        if (!e) break;

        g_now = e->time;       // jump simulated time
        e->handler(e);         // execute event-specific function
        free(e);               // destroy the event structure
    }
printf("\nSimulation finished\n");

int logical_packets = g_sender.sent + g_sender.lost_local;
int unique_ok       = g_receiver.unique_ok;
int total_delivs    = g_receiver.received_ok;
int retransmissions = total_delivs - unique_ok;

printf("Sender logical packets   : %d\n", logical_packets);
printf("  - scheduled to network : %d\n", g_sender.sent);
printf("  - local drops          : %d\n", g_sender.lost_local);

printf("Receiver unique packets  : %d\n", unique_ok);
printf("Receiver total deliveries: %d\n", total_delivs);
printf("Retransmissions received : %d\n", retransmissions);

double avg_delay = (g_net.count_delay > 0)
    ? (g_net.sum_delay / g_net.count_delay)
    : 0.0;
printf("Average one-way network delay: %.6f s (from %d deliveries)\n",
       avg_delay, g_net.count_delay);

}

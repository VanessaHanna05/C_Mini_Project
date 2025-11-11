#include <stdio.h>        // For printf (logging)
#include <stdlib.h>       // For malloc, free
#include "sender.h"       // Sender struct and prototypes
#include "heap_priority.h"// For schedule_event, Event

/*
We use g_now (current simulation time) defined in main.c.
*/
extern double g_now;

/*
P_LOST_EVENT:
- Probability that a DATA packet is "lost" before even being scheduled.
- Here set to 0.03 = 3% chance per packet.
*/
#define P_LOST_EVENT 0.03

/*
SYN_RTO:
- Retransmission timeout in seconds for the SYN handshake packet.
- If we haven't received SYNACK within this time, we retransmit SYN.
*/
#define SYN_RTO 0.5

/*
frand01:
- Helper function returning a random double in the range [0, 1].
- Used to decide probabilistic events like "lost before schedule".
*/
static double frand01(void) { 
    return rand() / (double)RAND_MAX; 
}

/*
new_int_heap:
- Allocates an int on the heap (dynamic memory).
- Stores the value v in that allocated memory.
- Returns the int* so it can be used as Event->data payload.

Why:
- We cannot safely store addresses of local variables in Event->data,
  because they go out of scope.
- Using heap ensures the memory stays valid until we explicitly free it.
*/
int* new_int_heap(int v) {
    int *p = malloc(sizeof(int)); // allocate space for one int
    *p = v;                       // store the given value
    return p;                     // return pointer so caller can attach to event
}

/*
sender_init:
- Sets up the initial state of the Sender and schedules the first events.

Parameters:
- s               : pointer to Sender struct.
- id              : unique identifier for this sender (should be 0 in this program).
- send_interval_s : interval between data packets in seconds.
- duration_s      : total sending duration in seconds.

Steps:
1. Initialize all counters and timing parameters.
2. Initialize acked[] array to 0 (no packet is ACKed initially).
3. Schedule EVT_SEND_SYN at time 0.0 to start the handshake.
4. Schedule EVT_TIMEOUT at time SYN_RTO, carrying packet id 0, to monitor SYN.
*/
void sender_init(Sender *s, int id, double send_interval_s, double duration_s) {
    // 1. Basic identification and counters.
    s->id                  = id;        // set sender id
    s->sent                = 0;         // no DATA packets sent yet
    s->lost_events_counter = 0;         // no pre-schedule losses yet
    s->empty_counter       = 0;         // unused, but initialize to 0
    s->drop_counter        = 0;         // unused, but initialize to 0

    // 2. Timing configuration for sending behavior.
    s->next_send_time = 0.0;            // we can start sending right away
    s->send_interval  = send_interval_s;// store interval between sends
    s->duration       = duration_s;     // total time we will be sending data

    // 3. Packet id management.
    s->next_pkt_id = 0;                 // first data packet will have id 0

    // 4. Initialize ACK tracking array to 0 (no packet acked).
    for (int i = 0; i < MAX_PKTS; ++i)
        s->acked[i] = 0;

    // 5. Schedule the first SYN send event at time 0.0.
    //    src = s->id (sender), dst = 1 (receiver's id).
    schedule_event(0.0,        // at time 0 (start of simulation)
                   EVT_SEND_SYN,
                   s->id,      // src = sender
                   1,          // dst = receiver with id 1
                   NULL);      // no extra data needed for SYN

    // 6. Schedule a timeout to monitor the SYN; if we don't get SYNACK by
    //    SYN_RTO seconds, we will retransmit SYN.
    schedule_event(SYN_RTO,    // time when the timeout "fires"
                   EVT_TIMEOUT,
                   s->id,      // src = sender itself
                   s->id,      // dst = also sender (timeout handled by sender)
                   new_int_heap(0)); // data = packet id 0 (SYN)
}

/*
sender_handle:
- Dispatches and handles all event types that belong to the sender.

Parameters:
- s   : pointer to Sender state.
- net : pointer to Network model.
- e   : pointer to current Event popped from queue.

Handled event types:
- EVT_SEND_SYN
- EVT_RECV_SYNACK
- EVT_SEND_DATA
- EVT_RECV_DATA_ACK
- EVT_TIMEOUT
*/
void sender_handle(Sender *s, Network* net, Event *e) {
    switch (e->type) {

    case EVT_SEND_SYN:
        /*
        EVT_SEND_SYN:
        - This event tells the sender to send a SYN to the receiver
          as the first step of connection establishment.
        - We:
            1. Log that we are sending SYN at g_now.
            2. Use network_schedule_delivery to schedule EVT_RECV_SYN
               at the receiver side, with appropriate delay.
        */
        printf("[%.3f] Sender: SEND SYN\n", g_now);

        // Schedule SYN arrival at the receiver via the network.
        network_schedule_delivery(net,
                                  g_now,         // send time = now
                                  EVT_RECV_SYN,  // receiver will handle EVT_RECV_SYN
                                  s->id,         // src = sender id
                                  e->dst,        // dst = receiver id (1)
                                  NULL);         // no extra data for SYN
        break;

    case EVT_RECV_SYNACK:
        /*
        EVT_RECV_SYNACK:
        - The sender has received a SYNACK from the receiver (step 2 of handshake).
        - We:
            1. Log that SYNACK arrived and that we will send ACK and start data.
            2. Mark acked[0] = 1 to indicate that packet id 0 (SYN) is acknowledged.
            3. Send an ACK back to the receiver (EVT_RECV_ACK).
            4. Decide whether to start sending DATA or directly finish:
               * If g_now + send_interval <= duration, schedule EVT_SEND_DATA.
               * Otherwise, schedule EVT_RECV_FINISH at the receiver.
        */
        printf("[%.3f] Sender: RECV SYNACK -> SEND ACK, start data\n", g_now);

        // Mark handshake packet (id 0) as acknowledged.
        s->acked[0] = 1;

        // Send final ACK to complete handshake, delivered to receiver.
        network_schedule_delivery(net,
                                  g_now,        // send ACK now
                                  EVT_RECV_ACK, // event type handled by receiver
                                  s->id,        // src = sender
                                  e->dst,       // dst = receiver
                                  NULL);        // no extra data

        // Decide whether there is still time to send data.
        if (g_now + s->send_interval <= s->duration) {
            // Schedule the first DATA send event in the future.
            schedule_event(g_now + s->send_interval,
                           EVT_SEND_DATA,
                           s->id,   // src = sender
                           e->dst,  // dst = receiver
                           NULL);
        } else {
            // No time left to send data, so finish the simulation at receiver.
            schedule_event(g_now,
                           EVT_RECV_FINISH,
                           s->id,   // src = sender
                           e->dst,  // dst = receiver
                           NULL);
        }
        break;

    case EVT_SEND_DATA: {
        /*
        EVT_SEND_DATA:
        - This event tells the sender to send one DATA packet.

        Steps:
        1. If g_now > duration, no more data should be sent:
           -> schedule EVT_RECV_FINISH at the receiver and return.
        2. Assign a packet id = next_pkt_id++, so each packet gets a unique id.
        3. With probability P_LOST_EVENT, simulate an immediate loss:
             * increment lost_events_counter,
             * log the loss,
             * DO NOT schedule anything into the network.
           Else:
             * log sending the packet,
             * schedule EVT_RECV_DATA at the receiver with payload = pkt_id,
             * increment sent counter.
        4. Compute nxt = g_now + send_interval:
           - If nxt <= duration, schedule another EVT_SEND_DATA at nxt.
           - Else, schedule EVT_RECV_FINISH to tell receiver we are done.
        */

        // 1. Check if we've already passed the allowed sending duration.
        if (g_now > s->duration) {
            // No more data allowed; send finish message instead.
            network_schedule_delivery(net,
                                      g_now,
                                      EVT_RECV_FINISH,
                                      s->id,
                                      e->dst,
                                      NULL);
            break;
        }

        // 2. Assign a new packet id and increment next_pkt_id for next time.
        int pkt_id = s->next_pkt_id++;

        // 3. Simulate pre-schedule packet loss using random probability.
        if (frand01() < P_LOST_EVENT) {
            // Pre-schedule loss: packet never enters the event queue.
            s->lost_events_counter++;
            printf("[%.3f] Sender: LOST pkt #%d before schedule\n", g_now, pkt_id);
        } else {
            // Packet is successfully scheduled through the network.
            printf("[%.3f] Sender: SEND DATA #%d\n", g_now, pkt_id);

            // Schedule EVT_RECV_DATA at the receiver with payload = pkt_id.
            network_schedule_delivery(net,
                                      g_now,           // send timestamp
                                      EVT_RECV_DATA,   // receiver event type
                                      s->id,           // src = sender
                                      e->dst,          // dst = receiver
                                      new_int_heap(pkt_id)); // payload: new int containing id
            s->sent++; // count this as a successfully scheduled DATA packet
        }

        // 4. Schedule next data send or finish.
        double nxt = g_now + s->send_interval;

        if (nxt <= s->duration) {
            // We still have time to send another DATA packet.
            schedule_event(nxt,
                           EVT_SEND_DATA,
                           s->id,   // src = sender
                           e->dst,  // dst = receiver
                           NULL);
        } else {
            // No sending time left; notify receiver that we are done.
            network_schedule_delivery(net,
                                      g_now,
                                      EVT_RECV_FINISH,
                                      s->id,
                                      e->dst,
                                      NULL);
        }
        break;
    }

    case EVT_RECV_DATA_ACK: {
        /*
        EVT_RECV_DATA_ACK:
        - Sender has received an ACK for a DATA packet sent earlier.

        Steps:
        1. Extract packet id from e->data (which should be int*).
        2. If pkt_id is valid (0 <= id < MAX_PKTS):
           - Mark s->acked[pkt_id] = 1.
           - Log that the ACK was received.
        3. Free e->data because we allocated it using new_int_heap on receiver side.
        */
        int pkt_id = e->data ? *(int*)e->data : -1;

        // Check that id is within array bounds.
        if (pkt_id >= 0 && pkt_id < MAX_PKTS) {
            s->acked[pkt_id] = 1; // mark the corresponding packet as acknowledged
            printf("[%.3f] Sender: RECV ACK for pkt #%d\n", g_now, pkt_id);
        }

        // Free the heap memory assigned to this payload.
        free(e->data);
        break;
    }

    case EVT_TIMEOUT: {
        /*
        EVT_TIMEOUT:
        - Timer event, used here for SYN retransmission only.

        Steps:
        1. Extract 'id' from e->data (int*).
        2. If id == 0 (SYN) and we still have not received acked[0],
           - Log that SYN timed out.
           - Immediately schedule a new EVT_SEND_SYN.
           - Schedule another EVT_TIMEOUT at g_now + SYN_RTO.
        3. Free e->data to avoid memory leak.
        */
        int id = e->data ? *(int*)e->data : -1;

        // Handle SYN timeout only if handshake not complete.
        if (id == 0 && !s->acked[0]) {
            printf("[%.3f] Sender: SYN timeout → retransmit\n", g_now);

            // Retransmit SYN right now.
            schedule_event(g_now,
                           EVT_SEND_SYN,
                           s->id, // src = sender
                           1,     // dst = receiver id (1)
                           NULL);

            // Schedule another timeout for this retransmitted SYN.
            schedule_event(g_now + SYN_RTO,
                           EVT_TIMEOUT,
                           s->id,       // src = sender
                           s->id,       // dst = sender (timeout handled locally)
                           new_int_heap(0)); // payload = SYN id (0)
        }

        // Free timeout payload memory.
        free(e->data);
        break;
    }

    default:
        /*
        Any event types not explicitly handled by the sender are ignored here.
        This yields a no-op for unknown/irrelevant events.
        */
        break;
    }
}

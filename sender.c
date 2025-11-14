#include <stdio.h>
#include <stdlib.h>
#include "sender.h"
#include "globals.h"   // provides G_NET/G_RCV globals for wiring peers
#include "network.h"
#include "receiver.h"  /* for receiver handler prototypes */

extern double g_now;   // global simulation clock (set by main loop before each event)

/*
 * -----------------------------------------------------------------------------
 *  MODULE PURPOSE (Sender)
 * -----------------------------------------------------------------------------
 *  The Sender is the traffic generator and simple reliability shim.
 *  Responsibilities:
 *   - Initiate a connection (SYN), complete handshake (upon SYNACK, send ACK)
 *   - Generate data packets at a fixed interval until a stop time
 *   - Model a small chance of LOCAL loss (before the packet even hits the network)
 *   - Handle ACKs for data packets and mark them as received
 *   - Retransmit SYN if the handshake times out (simple watchdog)
 *
 *  Notably *not* modeled here (kept intentionally simple):
 *   - Congestion control, sliding windows, retransmission of DATA
 *   - Reordering, duplicate suppression, selective ACK, etc.
 * -----------------------------------------------------------------------------
 */

/* Tunables:
 * - P_LOST_EVENT: probability the sender "loses" an event before scheduling it
 *                 (models local issues; separate from network delay/loss models).
 * - SYN_RTO:      retransmission timeout for the handshake SYN (seconds). */
#define P_LOST_EVENT 0.03
#define SYN_RTO 0.5

/* Private helper for uniform randoms (0..1) and a payload destructor alias. */
static double frand01(void){ return rand()/(double)RAND_MAX; }
static void free_int(void *p){ free(p); }

/* new_int_heap:
 * --------------
 * Allocate an int on the heap, initialize it to v, and return the pointer.
 * We use heap-allocated payloads because each scheduled event owns its own data,
 * and the main loop will free payloads via the event's data_dtor. */
int* new_int_heap(int v){
    int *p = (int*)malloc(sizeof(int));
    *p = v;
    return p;
}

/*
 * sender_init:
 * ------------
 * Initialize sender state and kick off the protocol by scheduling:
 *   1) A SYN send (time = 0)
 *   2) A watchdog timeout for the SYN (time = SYN_RTO)
 *
 * Why schedule here (and not in main)?
 *   - Encapsulation: main() shouldn’t know protocol internals.
 *   - Testability: unit tests can init a sender and let it schedule itself.
 *
 * Important fields:
 *   - send_interval: spacing between data packets (seconds)
 *   - duration:      absolute stop time for sending (seconds, sim time)
 *   - next_pkt_id:   monotonically increasing packet id (for ACK correlation)
 *   - acked[]:       marks whether each pkt_id has been acknowledged (0/1)
 */
void sender_init(Sender *s, int id, double send_interval_s, double duration_s) {
    s->id = id;
    s->sent = 0;
    s->lost_events_counter = 0;
    s->empty_counter = 0;    // kept for parity with your stats; not used by logic here
    s->drop_counter = 0;     // kept for parity; not used by logic here
    s->next_send_time = 0.0; // explicit next-send marker (not strictly required)
    s->send_interval = send_interval_s;
    s->duration = duration_s;
    s->next_pkt_id = 0;
    for (int i = 0; i < MAX_PKTS; ++i) s->acked[i] = 0;

    /* Kick off the 3-way handshake:
     *  - Immediately try to send SYN to receiver (dst id 1 in this topology)
     *  - Arm a timeout; if SYNACK doesn't arrive by SYN_RTO, retransmit SYN
     *
     * Note: These are scheduled as events so the main loop processes them in
     *       timestamp order alongside all other events. */
    schedule_event(0.0,      s->id, /*dst*/ 1, NULL, NULL,                 snd_send_syn, s);
    schedule_event(SYN_RTO,  s->id, s->id,   new_int_heap(0), free_int,    snd_timeout,  s);
}

/* -------------------------------------------------------------------------- */
/* Event Handlers                                                             */
/* -------------------------------------------------------------------------- */

/*
 * snd_send_syn:
 * -------------
 * “Send” the SYN by scheduling a receive on the receiver’s side via the network.
 * We do not call receiver functions directly; instead we go through the Network
 * layer so that delays/jitter apply uniformly, and delivery is time-ordered.
 */
void snd_send_syn(void *ctx, Event *e){
    Sender *s = (Sender*)ctx;
    printf("[%.3f] Sender: SEND SYN\n", g_now);

    /* network_schedule_delivery:
     * - Draws a random delay (base ± jitter)
     * - Schedules rcv_recv_syn at the receiver’s context (G_RCV)
     * - Delivery time = g_now + delay
     *
     * e->dst holds the intended remote id (set at schedule time by the caller). */
    network_schedule_delivery(G_NET, g_now,
        s->id, /*dst id*/ e->dst,
        NULL, NULL,
        rcv_recv_syn, G_RCV);
}

/*
 * snd_recv_synack:
 * ----------------
 * Sender receives SYNACK (network delivered rcv->snd).
 * Actions:
 *   - Mark handshake as acknowledged (acked[0]=1) to satisfy the timeout watchdog.
 *   - Send final ACK to the receiver (completes handshake).
 *   - Schedule first data send if there’s still time left; otherwise ask receiver
 *     to finish immediately.
 *
 * Why acked[0]?
 *   - We treat index 0 as the handshake “packet id” for the timeout logic.
 *     If it’s still 0 when the timer fires, we retransmit SYN.
 */
void snd_recv_synack(void *ctx, Event *e){
    Sender *s = (Sender*)ctx;
    printf("[%.3f] Sender: RECV SYNACK -> SEND ACK, start data\n", g_now);
    s->acked[0] = 1;  // the SYN was acknowledged; cancel effect for timeout handler

    /* Send ACK back to receiver (no payload needed). */
    network_schedule_delivery(G_NET, g_now,
        s->id, e->dst,
        NULL, NULL,
        rcv_recv_ack, G_RCV);

    /* Start the data phase:
     *  - If the next send time (now + interval) is within duration, schedule data
     *  - Otherwise, tell the receiver we’re done (no data to send) */
    if (g_now + s->send_interval <= s->duration) {
        schedule_event(g_now + s->send_interval, s->id, e->dst, NULL, NULL, snd_send_data, s);
    } else {
        schedule_event(g_now, s->id, e->dst, NULL, NULL, rcv_recv_finish, G_RCV);
    }
}

/*
 * snd_send_data:
 * --------------
 * Emit one data packet (or drop it locally with small probability),
 * then schedule the *next* data send if we haven’t reached the duration limit.
 *
 * Control flow:
 *   1) If current time > duration → we’re late; tell receiver to finish.
 *   2) Generate a pkt_id and increment the counter.
 *   3) With probability P_LOST_EVENT, simulate a local drop (before network).
 *      Otherwise, schedule delivery to receiver with payload = pkt_id.
 *   4) Compute next send time; if still within duration, schedule another
 *      snd_send_data at that time. Else, schedule receiver finish.
 *
 * Notes:
 *   - Local loss is separate from any network loss model; useful to demonstrate
 *     the difference between “didn’t schedule at all” vs “scheduled but delayed/lost later.”
 */
void snd_send_data(void *ctx, Event *e){
    Sender *s = (Sender*)ctx;

    /* Guard: events can arrive slightly out-of-window due to jittered scheduling.
     * If we’ve passed the sender’s stop time, just wrap up. */
    if (g_now > s->duration) {
        network_schedule_delivery(G_NET, g_now,
            s->id, e->dst,
            NULL, NULL,
            rcv_recv_finish, G_RCV);
        return;
    }

    int pkt_id = s->next_pkt_id++;   // allocate a unique id for this packet

    /* Local “pre-network” loss model:
     * - If we drop here, nothing is scheduled on the network side. */
    if (frand01() < P_LOST_EVENT) {
        s->lost_events_counter++;
        printf("[%.3f] Sender: LOST pkt #%d before schedule\n", g_now, pkt_id);
    } else {
        printf("[%.3f] Sender: SEND DATA #%d\n", g_now, pkt_id);
        network_schedule_delivery(G_NET, g_now,
            s->id, e->dst,
            new_int_heap(pkt_id), free_int,   // payload = pkt_id, main loop will free it
            rcv_recv_data, G_RCV);
        s->sent++;                   // count only packets we actually scheduled
    }

    /* Self-schedule the next data emission to keep the stream going. */
    double nxt = g_now + s->send_interval;
    if (nxt <= s->duration) {
        schedule_event(nxt, s->id, e->dst, NULL, NULL, snd_send_data, s);
    } else {
        /* We’re done sending; notify receiver so it can stop the simulation. */
        network_schedule_delivery(G_NET, g_now,
            s->id, e->dst,
            NULL, NULL,
            rcv_recv_finish, G_RCV);
    }
}

/*
 * snd_recv_data_ack:
 * ------------------
 * Receiver has acknowledged a data packet. Update bookkeeping so that
 * upper layers (or future extensions) could know delivery status.
 *
 * Ownership:
 *  - The main loop frees e->data (the acked pkt id) after this handler returns,
 *    via the event’s data_dtor (free). */
void snd_recv_data_ack(void *ctx, Event *e){
    Sender *s = (Sender*)ctx;
    int pkt_id = e->data ? *(int*)e->data : -1;  // defensive: handle NULL payload
    if (pkt_id >= 0 && pkt_id < MAX_PKTS) {
        s->acked[pkt_id] = 1;
        printf("[%.3f] Sender: RECV ACK for pkt #%d\n", g_now, pkt_id);
    }
}

/*
 * snd_timeout:
 * ------------
 * Handshake watchdog: fires at g_now == SYN_RTO (or later, if delayed).
 * If we still haven’t recorded the handshake ACK (acked[0] == 0),
 * retransmit SYN and arm another timeout.
 *
 * Notes:
 *  - In more complete designs, you would cap the number of retries,
 *    backoff the timeout (exponential RTO), or abort after some attempts.
 *  - Here we keep it simple to spotlight the event-driven pattern.
 */
void snd_timeout(void *ctx, Event *e){
    Sender *s = (Sender*)ctx;
    int id = e->data ? *(int*)e->data : -1; // which timer fired (0 = handshake timer)
    if (id == 0 && !s->acked[0]) {
        printf("[%.3f] Sender: SYN timeout → retransmit\n", g_now);

        /* Retransmit the SYN now, and schedule the next timeout window. */
        schedule_event(g_now,           s->id, /*dst*/ 1, NULL, NULL,                 snd_send_syn, s);
        schedule_event(g_now + SYN_RTO, s->id, s->id,   new_int_heap(0), free_int,    snd_timeout,  s);
    }
}

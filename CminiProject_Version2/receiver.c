#include <stdio.h>
#include "receiver.h"
#include "globals.h"   // gives access to global pointers: G_NET, G_SND (for event routing)
#include "network.h"
#include <stdlib.h>
#include "sender.h"    // allows us to call sender-side handlers like snd_recv_data_ack()

extern double g_now;   // global simulation clock — updated by main loop before handling events

/*
 * -----------------------------------------------------------------------------
 *  MODULE PURPOSE
 * -----------------------------------------------------------------------------
 *  The Receiver represents the “receiving end” of our simulated network.
 *  Its job:
 *   - Respond to connection setup messages (SYN, ACK)
 *   - Accept data packets
 *   - Send acknowledgments (ACKs) back to the sender
 *   - Detect the end of transmission (FINISH) and signal simulation termination
 *
 *  Unlike a real TCP receiver, this one does not buffer or reorder packets.
 *  The design is deliberately minimal to make the event-driven model clear.
 * -----------------------------------------------------------------------------
 */


/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

/*
 * receiver_init():
 * ----------------
 * This function initializes the receiver structure before simulation starts.
 *
 * Why we need this:
 *   - The receiver stores small internal statistics (counters for valid and invalid packets)
 *   - These are reset to zero at the start of every simulation
 *
 * Why no dynamic memory here:
 *   - The receiver struct is typically allocated once on the stack in main()
 *   - No heap allocation needed; keeps lifetime simple (ends with main())
 */
void receiver_init(Receiver *r, int id){
    r->id = id;                 // assign a unique logical identifier (used in network routing)
    r->received_ok = 0;         // counter of valid data packets received
    r->invalid_packets = 0;     // counter of malformed or unexpected packets
}


/* -------------------------------------------------------------------------- */
/* Connection setup (handshake phase)                                         */
/* -------------------------------------------------------------------------- */

/*
 * rcv_recv_syn():
 * ---------------
 * Triggered when the receiver “receives” a SYN event from the sender.
 * In real-world networking terms, this is like the first step of a TCP
 * three-way handshake where the sender says “I want to establish a connection.”
 *
 * Behavior:
 *   - Prints a log message showing the current simulation time (g_now)
 *   - Immediately schedules a SYNACK event back to the sender
 *
 * Design reasoning:
 *   - We don't maintain explicit connection states (like LISTEN, SYN_RCVD, etc.)
 *     because the goal here is to demonstrate event scheduling and delay modeling.
 *   - The “reply” (SYNACK) travels through the simulated Network layer, which
 *     applies a random delay (base_delay ± jitter).
 *
 * The scheduled event will invoke snd_recv_synack() on the sender side.
 */
void rcv_recv_syn(void *ctx, Event *e){
    Receiver *r = (Receiver*)ctx;  // convert generic ctx pointer to a Receiver*
    printf("[%.3f] Receiver: RECV SYN -> SEND SYNACK\n", g_now);

    /*
     * Send SYNACK back to sender:
     * ----------------------------
     * network_schedule_delivery() handles:
     *   - Picking a random propagation delay
     *   - Scheduling the receiver’s reply event (snd_recv_synack)
     *   - Ensuring it executes at (current time + delay)
     *
     * Parameters:
     *   G_NET:   pointer to the shared Network model
     *   g_now:   current simulation time (when we’re sending)
     *   r->id:   source of the packet (the receiver)
     *   e->src:  destination (the original sender who sent the SYN)
     *   data:    NULL (we don’t need to send any payload)
     *   dtor:    NULL (no data to free)
     *   handler: snd_recv_synack (what to run at destination)
     *   ctx:     G_SND (context for the handler: pointer to sender instance)
     */
    network_schedule_delivery(G_NET, g_now,
        r->id, /* dst */ e->src,
        NULL, NULL,
        snd_recv_synack, G_SND);
}


/*
 * rcv_recv_ack():
 * ---------------
 * This event represents the receiver receiving the final ACK from the sender,
 * which completes the 3-way handshake.
 *
 * Why this function is simple:
 *   - It just prints a message to show that the “connection” is established.
 *   - No further state tracking is done here because the simulation’s
 *     purpose is educational — to show the flow of events.
 */
void rcv_recv_ack(void *ctx, Event *e){
    (void)ctx;  // unused
    (void)e;    // unused
    printf("[%.3f] Receiver: RECV ACK (connection established)\n", g_now);
}


/* -------------------------------------------------------------------------- */
/* Data transfer phase                                                        */
/* -------------------------------------------------------------------------- */

/*
 * rcv_recv_data():
 * ----------------
 * Called when a data packet arrives from the sender.
 * Each data packet carries a small integer payload representing its ID.
 *
 * Steps:
 *   1. Validate that the packet contains data (the payload pointer is not NULL)
 *   2. Log the packet reception with its ID and the current simulation time
 *   3. Increment receiver’s “ok” counter
 *   4. Send back an ACK (acknowledgment) to the sender, containing the same ID
 *
 * Why send an ACK?
 *   - To confirm delivery to the sender, allowing it to mark the packet as received.
 *
 * Why allocate a new integer for the ACK?
 *   - Each event must have its own allocated payload because the simulator
 *     might free it independently (payloads can’t be shared between events).
 */
void rcv_recv_data(void *ctx, Event *e){
    Receiver *r = (Receiver*)ctx;   // cast context back to receiver
    int *pkt_id = (int*)e->data;    // retrieve payload (packet ID)
    if (!pkt_id) {
        // If the payload is missing, treat it as a corrupted event.
        r->invalid_packets++;
        printf("[%.3f] Receiver: received NULL data -> invalid packet\n", g_now);
        return;
    }

    printf("[%.3f] Receiver: RECV DATA #%d -> SEND ACK\n", g_now, *pkt_id);
    r->received_ok++;  // increment successful reception counter

    /*
     * Now we generate a new integer on the heap (fresh memory)
     * for the ACK packet’s payload.
     *
     * We use new_int_heap(*pkt_id) to allocate an int and copy the value.
     * We also specify free() as the destructor, since new_int_heap uses malloc().
     */
    network_schedule_delivery(G_NET, g_now,
        r->id, /* dst */ e->src,
        new_int_heap(*pkt_id), /* dtor */ free,
        snd_recv_data_ack, G_SND);

    /*
     * After this function returns:
     * - The simulator’s main loop will eventually free e->data if needed.
     * - The ACK event is now scheduled in the heap to be delivered later.
     */
}


/* -------------------------------------------------------------------------- */
/* Connection termination                                                     */
/* -------------------------------------------------------------------------- */

/*
 * rcv_recv_finish():
 * ------------------
 * This event marks the end of the simulation.
 * The sender schedules this event when all data packets have been sent.
 *
 * What it does:
 *   - Prints “done!” for clarity
 *   - Sets g_stop_simulation = 1, which instructs the main loop to exit
 *
 * Why not just exit() here?
 *   - Because the simulator’s design aims to centralize control flow in main().
 *     By using a flag, we allow main() to perform final cleanup and print stats.
 */
void rcv_recv_finish(void *ctx, Event *e){
    (void)ctx;  // not used
    (void)e;    // not used
    printf("[%.3f] Receiver: FINISH signal received -> stopping simulation\n", g_now);

    extern int g_stop_simulation;  // declared in main.c
    g_stop_simulation = 1;         // flag main loop to stop popping events
}

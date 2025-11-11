#include <stdio.h>       // For printf used in logging
#include <stdlib.h>      // For free (freeing packet ids)
#include "receiver.h"    // Receiver struct and prototypes
#include "heap_priority.h" // For schedule_event via network and Event usage

/*
We declare externs for the global simulation state, which are defined in main.c.
- g_now            : current simulation time (used in logs).
- g_stop_simulation: flag to signal main loop to stop.
*/
extern double g_now;
extern int    g_stop_simulation;

/*
Forward declaration of new_int_heap:
- This function is defined in sender.c but we use it here to allocate
  a heap-based int to send back to the sender as ACK payload.
*/
int* new_int_heap(int v);

/*
receiver_init:
- Initializes the receiver structure fields.

Parameters:
- r  : pointer to the Receiver to initialize.
- id : id to assign to this receiver (e.g., 1).

Behavior:
- Sets r->id to the given id.
- Resets the counters received_ok and invalid_packets to zero.
*/
void receiver_init(Receiver *r, int id) {
    r->id              = id; // keep track of who this receiver is
    r->received_ok     = 0;  // initially no valid packets received
    r->invalid_packets = 0;  // initially no invalid packets counted
}

/*
receiver_handle:
- Central handler for all events that are destined to the Receiver.

Parameters:
- r   : pointer to Receiver state (counters, id).
- net : pointer to Network model to send responses.
- e   : the Event that needs to be processed.

We switch on e->type to select behavior for:
- EVT_RECV_SYN
- EVT_RECV_ACK
- EVT_RECV_DATA
- EVT_RECV_FINISH
*/
void receiver_handle(Receiver *r, Network* net, Event *e) {
    switch (e->type) {

    case EVT_RECV_SYN:
        /*
        EVT_RECV_SYN:
        - This event means the receiver "sees" a SYN from the sender
          (step 1 of the handshake).
        - We log the reception along with the current simulation time.
        - Then we respond by scheduling EVT_RECV_SYNACK back to the sender
          through the network, using network_schedule_delivery.
        */
        printf("[%.3f] Receiver: RECV SYN -> SEND SYNACK\n", g_now);

        // Schedule SYNACK to be received by the sender after network delay.
        // src = r->id (receiver), dst = e->src (sender).
        network_schedule_delivery(net,
                                  g_now,          // send time = now
                                  EVT_RECV_SYNACK,// event type the sender will handle
                                  r->id,          // src id (receiver)
                                  e->src,         // dst id (sender)
                                  NULL);          // no extra data payload
        break;

    case EVT_RECV_ACK:
        /*
        EVT_RECV_ACK:
        - Receiver got the final ACK of the three-way handshake.
        - This confirms that the connection is established.
        - We only print a log line; no further scheduling is necessary here.
        */
        printf("[%.3f] Receiver: RECV ACK (established)\n", g_now);
        break;

    case EVT_RECV_DATA: {
        /*
        EVT_RECV_DATA:
        - Receiver got a DATA packet from sender via the network.
        - The packet id is stored in e->data as an int*.

        Steps:
        1. Cast e->data to int* and check it's not NULL.
        2. Log the reception of the packet, including its id.
        3. Increment the received_ok counter.
        4. Schedule an ACK (EVT_RECV_DATA_ACK) back to the sender, carrying a
           fresh heap-allocated copy of the same packet id.
        5. Free the original pkt_id pointer received in e->data, since it
           belongs to this event and we have already used its information.
        */
        int *pkt_id = (int*)e->data; // interpret payload as pointer to packet id

        // If somehow data was NULL, we cannot process this packet; just break.
        if (!pkt_id) break;

        // 2. Log the successful reception of the data packet with its id.
        printf("[%.3f] Receiver: RECV DATA #%d -> ACK\n", g_now, *pkt_id);

        // 3. Count this packet as correctly received.
        r->received_ok++;

        // 4. Schedule an ACK event to be delivered at the sender side
        //    (event type EVT_RECV_DATA_ACK).
        //    We allocate a fresh int on the heap to carry the packet id back.
        network_schedule_delivery(net,
                                  g_now,              // send ACK now (network adds delay)
                                  EVT_RECV_DATA_ACK,  // event type at sender side
                                  r->id,              // src (receiver)
                                  e->src,             // dst (sender)
                                  new_int_heap(*pkt_id)); // payload: copy of packet id

        // 5. We are done with this pkt_id memory from the received event, so free it.
        free(pkt_id);
        break;
    }

    case EVT_RECV_FINISH:
        /*
        EVT_RECV_FINISH:
        - Sender signalled that there is no more data to send.
        - This event is used to stop the simulation.
        - We:
            1. Print "done!".
            2. Set g_stop_simulation = 1 so that the main loop will exit.
        */
        printf("done!\n");
        g_stop_simulation = 1; // main simulation loop will stop after this event
        break;

    default:
        /*
        Any other event types are simply ignored by the receiver.
        This default case catches event types that are not handled here.
        */
        break;
    }
}

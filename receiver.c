#include <stdio.h>
#include "receiver.h"
#include "globals.h"
#include "network.h"
#include "sender.h"

// id: 0
// received_ok: total packets delivered here
// unique_ok: number of unique packets
// invalid_packets: packets with out-of-range IDs
// seen[]: array marking whether each packet ID was already received 
void receiver_init(Receiver *r, int id) {
    r->id              = id;
    r->received_ok     = 0; 
    r->unique_ok       = 0; 
    r->invalid_packets = 0; 
}

/*
   Handler for receiving a SYN packet during connection startup.*/
void rcv_recv_syn() {
    printf("[%.3f] Receiver: RECV SYN -> SEND SYNACK\n", g_now);
    // Schedule SYNACK to be delivered to the sender.
    network_schedule_delivery(&g_net,g_now, RECEIVER_ID, SENDER_ID,-1,snd_recv_synack);
}

/*Handler for receiving ACK after SYNACK.
   This confirms the connection is established.
   Just prints a message */
void rcv_recv_ack() {
   
    printf("[%.3f] Receiver: RECV ACK (connection established)\n", g_now);
}

//   Extract packet_id from the event.
//   Validate the ID (reject if out of bounds).
//   Count packet as received (received_ok++).
//   If never seen before, mark as unique (unique_ok++).
//   Print debug info.
//   Schedule an ACK back to the sender.

void rcv_recv_data(Event *e) {
    int pkt_id = e->packet_id;
    if (pkt_id < 0 || pkt_id >= RCV_MAX_PKTS) {
        g_receiver.invalid_packets++;
        printf("[%.3f] Receiver: RECV DATA with invalid id %d\n", g_now, pkt_id);
        return;
    }
    g_receiver.received_ok++;
    if (!g_receiver.seen[pkt_id]) {
        g_receiver.seen[pkt_id] = 1; 
        g_receiver.unique_ok++;     
    }

    printf("[%.3f] Receiver: RECV DATA #%d -> SEND ACK\n", g_now, pkt_id);
    network_schedule_delivery(&g_net, g_now, RECEIVER_ID,SENDER_ID,pkt_id, snd_recv_data_ack);
}

/*Handles FINISH event from sender indicating the end
   of the simulation. This is the final control message.
   Sets g_stop_simulation = 1, which causes the main event loop
   to exit and prints final statistics. */
void rcv_recv_finish() {
    printf("[%.3f] Receiver: RECV FINISH -> stop simulation\n", g_now);
    g_stop_simulation = 1; 
}

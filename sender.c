#include <stdio.h>
#include <stdlib.h>
#include "sender.h"
#include "globals.h"
#include "network.h"
#include "receiver.h"
#define RTO 0.05
// RTO = Retransmission TimeOut (seconds).
// If no ACK is received within RTO after sending a packet (or SYN),
// the sender will retransmit.

/*
     id             = SENDER_ID (0)
     send_interval_s= time between sending consecutive data packets
     duration_s     = total sending duration of the flow
 */
void sender_init(Sender *s, int id, double send_interval_s, double duration_s) {
    s->id            = id;                // logical ID used in events
    s->sent          = 0;                 // number of packets actually sent into the network
    s->lost_local    = 0;                 // number of packets dropped locally before network
    s->next_pkt_id   = 0;                 // next data packet ID to use (starts from 0)
    s->send_interval = send_interval_s;   // time between consecutive data sends
    s->duration      = duration_s;        // overall sending duration
    s->syn_acked     = 0;                 // 0 until SYNACK is received

    // Schedule sending of SYN at time 0.0.
    // This starts the connection handshake.
    schedule_event(0.0,  s->id, RECEIVER_ID, -1, snd_send_syn);
    // Schedule a timeout event for the SYN.
    // If no SYNACK is received by g_now + RTO, snd_timeout will be called.
    schedule_event(RTO,  s->id, s->id, -1, snd_timeout);
}


//  Ask the network to deliver a SYN to the receive by scheduling an event with handler rcv_recv_syn.
void snd_send_syn() {
    printf("[%.3f] Sender: SEND SYN\n", g_now);
    network_schedule_delivery(&g_net, g_now, SENDER_ID, RECEIVER_ID, -1, rcv_recv_syn);
}

    //  1) Mark syn_acked = 1 (handshake success).
    //  2) Log that SYNACK was received.
    //  3) Send an ACK back to the receiver to complete handshake.
    //  4) either schedule the first data send or, if duration already passed, schedule finish.
void snd_recv_synack() {
    g_sender.syn_acked = 1;  

    printf("[%.3f] Sender: RECV SYNACK -> SEND ACK, start data\n", g_now);
    network_schedule_delivery(&g_net, g_now, SENDER_ID, RECEIVER_ID, -1, rcv_recv_ack);
    double first_time = g_now + g_sender.send_interval;
    if (first_time <= g_sender.duration) {
        schedule_event(first_time, SENDER_ID, RECEIVER_ID, -1, snd_send_data);
    } else {
        schedule_event(g_now, SENDER_ID, RECEIVER_ID, -1, rcv_recv_finish);
    }
}



    //  1) Check if we've exceeded the duration If yes: schedule FINISH and stop sending new data.
    //  2) Allocate a new packet ID.
    //  3) Randomly decide if the packet is dropped.
    //  4) If not dropped: ask network to deliver DATA to receive +  increment s->sent
    //  5) Schedule a timeout for this packet (snd_timeout).
    //  6) Schedule the NEXT snd_send_data if still within duration else schedule FINISH.
void snd_send_data() {

    if (g_now > g_sender.duration) {
        schedule_event(g_now, SENDER_ID, RECEIVER_ID, -1, rcv_recv_finish);
        return;
    }
    int pkt_id = g_sender.next_pkt_id++;  // the packet ids are incremnental 

    if (frand01() < 0.08) { 
        g_sender.lost_local++;
        printf("[%.3f] Sender: LOCAL DROP of pkt #%d\n", g_now, pkt_id);
    } else {
        printf("[%.3f] Sender: SEND DATA #%d\n", g_now, pkt_id);
        network_schedule_delivery( &g_net, g_now,SENDER_ID, RECEIVER_ID, pkt_id,rcv_recv_data );
        g_sender.sent++;
    }
    schedule_event(g_now + RTO, SENDER_ID, SENDER_ID, pkt_id, snd_timeout);
    double next_time = g_now + g_sender.send_interval;

    if (next_time <= g_sender.duration) {
        // Schedule the sender to send another data packet later.
        schedule_event(next_time, SENDER_ID, RECEIVER_ID, -1, snd_send_data);
    } else {
        schedule_event(g_now, SENDER_ID, RECEIVER_ID, -1, rcv_recv_finish);
    }
}



    //  1) Read the packet_id from the event.
    //  2) If valid, mark it as acknowledged.
    //  3) Print a log message.

void snd_recv_data_ack(Event *e) {
    int pkt_id = e->packet_id;
    if (pkt_id >= 0 && pkt_id < MAX_PKTS) {
        g_sender.acked[pkt_id] = 1; 
        printf("[%.3f] Sender: RECV ACK for pkt #%d\n", g_now, pkt_id);
    } else {
        printf("[%.3f] Sender: RECV ACK with invalid pkt id %d\n", g_now, pkt_id);
    }
}




//   If pkt_id == -1: Handle SYN timeout ( retransmit SYN).
//   Else:If packet not yet ACKed, retransmit it and reschedule timeout.

void snd_timeout(Event *e) {
    int pkt_id = e->packet_id;

    //  SYN timeout (packet_id == -1)
    if (pkt_id == -1) {
        // Only retransmit if handshake hasn't completed yet.
        if (!g_sender.syn_acked) {
            printf("[%.3f] Sender: SYN TIMEOUT -> retransmit SYN\n", g_now);
            schedule_event(g_now,SENDER_ID, RECEIVER_ID, -1, snd_send_syn);
            schedule_event(g_now + RTO, SENDER_ID, SENDER_ID,   -1, snd_timeout);
        }
    } else {
        //  DATA packet timeout
        if (pkt_id >= 0 && pkt_id < MAX_PKTS && !g_sender.acked[pkt_id]) {
            printf("[%.3f] Sender: TIMEOUT pkt #%d -> retransmit\n", g_now, pkt_id);
            network_schedule_delivery(&g_net,g_now,SENDER_ID,RECEIVER_ID,pkt_id,rcv_recv_data);
            schedule_event(g_now + RTO, SENDER_ID, SENDER_ID, pkt_id, snd_timeout);
        }
    }
}

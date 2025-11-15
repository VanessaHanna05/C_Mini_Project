#include <stdio.h>
#include <stdlib.h>
#include "sender.h"
#include "globals.h"
#include "network.h"
#include "receiver.h"

extern double g_now;

/* Timeout for both SYN and DATA retransmissions (seconds). */
#define RTO 0.05

/* Helper for local (pre-network) random losses. */
static double frand01(void) {
    return rand() / (double)RAND_MAX;
}

/* Initialize sender fields and kick off the handshake. */
void sender_init(Sender *s, int id, double send_interval_s, double duration_s) {
    int i;

    s->id            = id;
    s->sent          = 0;
    s->lost_local    = 0;
    s->next_pkt_id   = 0;
    s->send_interval = send_interval_s;
    s->duration      = duration_s;
    s->syn_acked     = 0;

    /* At the start, no packet is acknowledged. */
    for (i = 0; i < MAX_PKTS; ++i) {
        s->acked[i] = 0;
    }

    /* Schedule initial SYN send and its timeout.
     * We use packet_id = -1 for handshake timeouts to distinguish them from data. */
    schedule_event(0.0,  s->id, RECEIVER_ID, -1, snd_send_syn);
    schedule_event(RTO,  s->id, s->id,       -1, snd_timeout);
}

/* Send a SYN to the receiver by scheduling a receive event via the network. */
void snd_send_syn(Event *e) {
    (void)e;  // we don't need fields from e here

    printf("[%.3f] Sender: SEND SYN\n", g_now);

    /* Deliver SYN to the receiver through the network (with random delay). */
    network_schedule_delivery(&g_net, g_now,
                              SENDER_ID, RECEIVER_ID,
                              -1,        // handshake has no packet id
                              rcv_recv_syn);
}

/* Handle reception of SYNACK from the receiver.
 * Actions:
 *   - Mark handshake as acknowledged (syn_acked = 1).
 *   - Send final ACK back to receiver.
 *   - Start sending data if we still have time before duration. */
void snd_recv_synack(Event *e) {
    (void)e;  // src/dst could be read from e, but not needed for logic

    g_sender.syn_acked = 1;
    printf("[%.3f] Sender: RECV SYNACK -> SEND ACK, start data\n", g_now);

    /* Send ACK back to receiver through the network. */
    network_schedule_delivery(&g_net, g_now,
                              SENDER_ID, RECEIVER_ID,
                              -1,
                              rcv_recv_ack);

    /* Schedule first data send if we still have time left. */
    double first_time = g_now + g_sender.send_interval;
    if (first_time <= g_sender.duration) {
        schedule_event(first_time, SENDER_ID, RECEIVER_ID, -1, snd_send_data);
    } else {
        /* No time left for data; tell receiver to finish. */
        schedule_event(g_now, SENDER_ID, RECEIVER_ID, -1, rcv_recv_finish);
    }
}

/* Send one data packet, possibly drop it locally, and schedule the next one. */
void snd_send_data(Event *e) {
    (void)e;  // we use global sender and g_now

    /* If the current time is already past the sender's duration, finish. */
    if (g_now > g_sender.duration) {
        schedule_event(g_now, SENDER_ID, RECEIVER_ID, -1, rcv_recv_finish);
        return;
    }

    int pkt_id = g_sender.next_pkt_id++;  // assign unique packet id

    /* Local drop model: sometimes the packet is "lost" before it hits network. */
    if (frand01() < 0.03) {   // 3% local loss
        g_sender.lost_local++;
        printf("[%.3f] Sender: LOCAL DROP of pkt #%d\n", g_now, pkt_id);
    } else {
        printf("[%.3f] Sender: SEND DATA #%d\n", g_now, pkt_id);

        /* Schedule delivery of data to the receiver. */
        network_schedule_delivery(&g_net, g_now,
                                  SENDER_ID, RECEIVER_ID,
                                  pkt_id,
                                  rcv_recv_data);

        /* Schedule a timeout for this specific packet id.
         * If it's not acked by then, we will retransmit in snd_timeout(). */
        

        g_sender.sent++;
    }

    schedule_event(g_now + RTO, SENDER_ID, SENDER_ID, pkt_id, snd_timeout);


    /* Schedule the next data send, or finish if duration is reached. */
    double next_time = g_now + g_sender.send_interval;
    if (next_time <= g_sender.duration) {
        schedule_event(next_time, SENDER_ID, RECEIVER_ID, -1, snd_send_data);
    } else {
        /* Done sending; let receiver know so it can stop the simulation. */
        schedule_event(g_now, SENDER_ID, RECEIVER_ID, -1, rcv_recv_finish);
    }
}

/* Handle an ACK for a data packet from the receiver. */
void snd_recv_data_ack(Event *e) {
    int pkt_id = e->packet_id;

    if (pkt_id >= 0 && pkt_id < MAX_PKTS) {
        g_sender.acked[pkt_id] = 1;  // mark this packet as acknowledged
        printf("[%.3f] Sender: RECV ACK for pkt #%d\n", g_now, pkt_id);
    } else {
        printf("[%.3f] Sender: RECV ACK with invalid pkt id %d\n", g_now, pkt_id);
    }
}

/* Timeout handler:
 *  - If e->packet_id == -1 -> handshake timeout (SYN).
 *  - If e->packet_id >= 0  -> data timeout for that specific packet id. */
void snd_timeout(Event *e) {
    int pkt_id = e->packet_id;

    if (pkt_id == -1) {
        /* Handshake timeout: SYN was not acknowledged in time. */
        if (!g_sender.syn_acked) {
            printf("[%.3f] Sender: SYN TIMEOUT -> retransmit SYN\n", g_now);

            /* Retransmit SYN now and schedule another handshake timeout. */
            schedule_event(g_now,       SENDER_ID, RECEIVER_ID, -1, snd_send_syn);
            schedule_event(g_now + RTO, SENDER_ID, SENDER_ID,   -1, snd_timeout);
        }
    } else {
        /* Data timeout: check if this packet is still unacked. */
        if (pkt_id >= 0 && pkt_id < MAX_PKTS && !g_sender.acked[pkt_id]) {
            printf("[%.3f] Sender: TIMEOUT pkt #%d -> retransmit\n", g_now, pkt_id);

            /* Retransmit the data packet and arm a new timeout. */
            network_schedule_delivery(&g_net, g_now,
                                      SENDER_ID, RECEIVER_ID,
                                      pkt_id,
                                      rcv_recv_data);
            schedule_event(g_now + RTO, SENDER_ID, SENDER_ID, pkt_id, snd_timeout);
        }
    }
}

#include <stdio.h>
#include <stdlib.h>
#include "sender.h"
#include "heap_priority.h"

extern double g_now;
#define P_LOST_EVENT 0.03
#define SYN_RTO 0.5

static double frand01(void) { return rand() / (double)RAND_MAX; }

 int* new_int_heap(int v) {
    int *p = malloc(sizeof(int));
    *p = v;
    return p;
}

void sender_init(Sender *s, int id, double send_interval_s, double duration_s) {
    s->id = id;
    s->sent = 0;
    s->lost_events_counter = 0;
    s->empty_counter = 0;
    s->drop_counter = 0;
    s->next_send_time = 0.0;
    s->send_interval = send_interval_s;
    s->duration = duration_s;
    s->next_pkt_id = 0;
    for (int i = 0; i < MAX_PKTS; ++i) s->acked[i] = 0;

    schedule_event(0.0, EVT_SEND_SYN, s->id, 1, NULL);
    schedule_event(SYN_RTO, EVT_TIMEOUT, s->id, s->id, new_int_heap(0));
}

void sender_handle(Sender *s, Network* net, Event *e) {
    switch (e->type) {
    case EVT_SEND_SYN: //: logs and uses the Network to schedule EVT_RECV_SYN at the Receiver
        printf("[%.3f] Sender: SEND SYN\n", g_now);
        network_schedule_delivery(net, g_now, EVT_RECV_SYN, s->id, e->dst, NULL);
        break;

    case EVT_RECV_SYNACK://ogs, sets acked[0] = 1 which marks handshake complete, schedules EVT_RECV_ACK to Receiver, 
    //and either schedules the first EVT_SEND_DATA at g_now + send_interval if still within duration, or schedules Receiver finish
        printf("[%.3f] Sender: RECV SYNACK -> SEND ACK, start data\n", g_now);
        s->acked[0] = 1; // mark handshake complete
        network_schedule_delivery(net, g_now, EVT_RECV_ACK, s->id, e->dst, NULL);
        if (g_now + s->send_interval <= s->duration)
            schedule_event(g_now + s->send_interval, EVT_SEND_DATA, s->id, e->dst, NULL);
        else
            schedule_event(g_now, EVT_RECV_FINISH, s->id, e->dst, NULL);
        break;

    case EVT_SEND_DATA: {
        //if current time exceeded duration, schedule EVT_RECV_FINISH to Receiver. 
        //Else pick pkt_id = next_pkt_id++. With probability 3 percent, increment lost_events_counter and 
        //do not schedule the network delivery. Otherwise schedule EVT_RECV_DATA carrying a heap allocated copy of 
        //pkt_id and increment sent. Then schedule the next EVT_SEND_DATA at g_now + send_interval if still within duration, 
        //otherwise schedule a Receiver finish.
        if (g_now > s->duration) {
            network_schedule_delivery(net, g_now, EVT_RECV_FINISH, s->id, e->dst, NULL);
            break;
        }
        int pkt_id = s->next_pkt_id++;
        if (frand01() < P_LOST_EVENT) {
            s->lost_events_counter++;
            printf("[%.3f] Sender: LOST pkt #%d before schedule\n", g_now, pkt_id);
        } else {
            printf("[%.3f] Sender: SEND DATA #%d\n", g_now, pkt_id);
            network_schedule_delivery(net, g_now, EVT_RECV_DATA, s->id, e->dst, new_int_heap(pkt_id));
            s->sent++;
        }
        double nxt = g_now + s->send_interval;
        if (nxt <= s->duration)
            schedule_event(nxt, EVT_SEND_DATA, s->id, e->dst, NULL);
        else
            network_schedule_delivery(net, g_now, EVT_RECV_FINISH, s->id, e->dst, NULL);
        break;
    }

    case EVT_RECV_DATA_ACK: {
        //read pkt_id from e->data, mark acked[pkt_id] = 1, log, then free e->data
        int pkt_id = e->data ? *(int*)e->data : -1;
        if (pkt_id >= 0 && pkt_id < MAX_PKTS) {
            s->acked[pkt_id] = 1;
            printf("[%.3f] Sender: RECV ACK for pkt #%d\n", g_now, pkt_id);
        }
        free(e->data);
        break;
    }

    case EVT_TIMEOUT: {
        //if it carries 0 and the handshake is not complete yet, retransmit EVT_SEND_SYN now and schedule another 
        //timeout at g_now + SYN_RTO, then free e->data
        int id = e->data ? *(int*)e->data : -1;
        if (id == 0 && !s->acked[0]) {
            printf("[%.3f] Sender: SYN timeout → retransmit\n", g_now);
            schedule_event(g_now, EVT_SEND_SYN, s->id, 1, NULL);
            schedule_event(g_now + SYN_RTO, EVT_TIMEOUT, s->id, s->id, new_int_heap(0));
        }
        free(e->data);
        break;
    }

    default:
        break;
    }
}

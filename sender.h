#ifndef SENDER_H
#define SENDER_H
#include "event.h"
#define MAX_PKTS 10000
#define SENDER_ID   0
#define RECEIVER_ID 1


typedef struct Sender {
    int    id;
    int    sent;
    int    lost_local;
    int    next_pkt_id;
    double send_interval;
    double duration;
    int    syn_acked;
    char   acked[MAX_PKTS];
} Sender;

void sender_init(Sender *s, int id, double send_interval_s, double duration_s);
void snd_send_syn();
void snd_recv_synack();
void snd_send_data();
void snd_recv_data_ack(struct Event *e);
void snd_timeout(struct Event *e);

#endif

#ifndef SENDER_H
#define SENDER_H


#include "event.h"
#include "network.h"


#define MAX_PKTS 10000


typedef struct {
int id;
int sent;
int lost_events_counter;
int empty_counter;
int drop_counter;
double next_send_time;
double send_interval;
double duration;
int next_pkt_id;
char acked[MAX_PKTS];
} Sender;


void sender_init(Sender *s, int id, double send_interval_s, double duration_s);
void sender_handle(Sender *s, Network* net, Event *e);


#endif
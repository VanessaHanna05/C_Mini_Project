#ifndef SENDER_H
#define SENDER_H

#include "event.h"

/* We assume at most this many packets will be sent. */
#define MAX_PKTS 10000

/* Logical ids for endpoints in the simulation. */
#define SENDER_ID   0
#define RECEIVER_ID 1

/* Sender state:
 *  - id             : logical id (0 in this topology)
 *  - sent           : how many DATA packets we actually scheduled
 *  - lost_local     : how many packets we "lost" before network (local drop)
 *  - next_pkt_id    : counter used to give each packet a unique id
 *  - send_interval  : time between packets (seconds)
 *  - duration       : stop sending after this absolute time (sim time)
 *  - syn_acked      : flag indicating if handshake SYN was acknowledged
 *  - acked[]        : acked[i] == 1 if data packet i was acknowledged
 */
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

/* Sender-owned event handlers. */
void snd_send_syn(struct Event *e);
void snd_recv_synack(struct Event *e);
void snd_send_data(struct Event *e);
void snd_recv_data_ack(struct Event *e);
void snd_timeout(struct Event *e);

#endif

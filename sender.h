#ifndef SENDER_H
#define SENDER_H
#include "event.h"

#define MAX_PKTS 10000  // cap for ack bookkeeping; large enough for demo runs

/*
 * Sender keeps simple traffic generation state and a small reliability shim.
 * We mark which packets were ACKed and use a timeout to retransmit SYN only
 * (data loss is modeled but not retransmitted to keep the example compact).
 */
typedef struct Sender {
    int    id;                    // endpoint id
    int    sent;                  // how many data packets we successfully scheduled
    int    lost_events_counter;   // how many were “lost” before scheduling
    int    empty_counter;         // placeholder stats (unused in this variant)
    int    drop_counter;          // placeholder stats (unused in this variant)
    double next_send_time;        // next time to emit data (redundant but explicit)
    double send_interval;         // spacing between packets (seconds)
    double duration;              // stop sending after this absolute time
    int    next_pkt_id;           // monotonically increasing packet id
    char   acked[MAX_PKTS];       // bitmap of which ids were ACKed (0/1)
} Sender;

void sender_init(Sender *s, int id, double send_interval_s, double duration_s);

/* Individual handlers wired into events (no central switch). */
void snd_send_syn(void *ctx, struct Event *e);
void snd_recv_synack(void *ctx, struct Event *e);
void snd_send_data(void *ctx, struct Event *e);
void snd_recv_data_ack(void *ctx, struct Event *e);
void snd_timeout(void *ctx, struct Event *e);

/* Utility to allocate an int payload on the heap — shared with receiver. */
int*  new_int_heap(int v);

#endif

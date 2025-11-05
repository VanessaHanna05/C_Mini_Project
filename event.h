#ifndef EVENT_H
#define EVENT_H

#include <stddef.h>

typedef enum {
    // handshake
    EVT_SEND_SYN = 1,
    EVT_RECV_SYN,
    EVT_SEND_SYNACK,
    EVT_RECV_SYNACK,
    EVT_SEND_ACK,
    EVT_RECV_ACK,

    // data
    EVT_SEND_DATA,
    EVT_RECV_DATA,
    EVT_SEND_DATA_ACK,
    EVT_RECV_DATA_ACK,

    // infra
    EVT_TIMEOUT,        // generic timeout (used for SYN RTO if you keep it)
    EVT_RECV_FINISH     // receiver should stop the simulation
} EventType;

/* NO Packet struct anymore */

typedef struct Event {
    double time;        // simulation time of event
    EventType type;
    int src;            // who scheduled / logical source
    int dst;            // intended logical target
    void *data;         // optional payload (now: int* random payload, or NULL)
} Event;

/* global simulation flags (defined in main.c) */
extern double g_now;
extern int g_stop_simulation;

/* scheduling API (provided by heap_priority.c) */
void schedule_event(double time, EventType type, int src, int dst, void *data);
struct Event* pop_next_event(void);
int event_queue_empty(void);
void event_queue_init(void);

#endif

#ifndef EVENT_H
#define EVENT_H


#include <stddef.h>


typedef enum {
EVT_SEND_SYN = 1,
EVT_RECV_SYN,
EVT_SEND_SYNACK,
EVT_RECV_SYNACK,
EVT_SEND_ACK,
EVT_RECV_ACK,
EVT_SEND_DATA,
EVT_RECV_DATA,
EVT_SEND_DATA_ACK,
EVT_RECV_DATA_ACK,
EVT_TIMEOUT,
EVT_RECV_FINISH
} EventType;


typedef struct Event {
double time;
EventType type;
int src;
int dst;
void *data; // new: for DATA packets, data is an int* (packet_id)
} Event;


extern double g_now;
extern int g_stop_simulation;


void schedule_event(double time, EventType type, int src, int dst, void *data);
struct Event* pop_next_event(void);
int event_queue_empty(void);
void event_queue_init(void);

#endif
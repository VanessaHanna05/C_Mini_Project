#ifndef EVENT_H
#define EVENT_H

/*

 * Each Event:
 *   - has a simulation time (time when it should run),
 *   - knows who scheduled it (src) and the logical destination (dst),
 *   -  carries a packet_id (for DATA/ACK/TIMEOUT),
 *   - and stores a function pointer to the handler to call.
 *
 * The main loop just:
 *   - pops the earliest event,
 *   - sets g_now to that time,
 *   - calls e->handler(e).
 */

struct Event;                     
// event handler is a function that is pointed by the EventHandler
typedef void (*EventHandler)(struct Event *e);

typedef struct Event {
    double       time;       // when to execute this event (simulation time)
    int          src;        // id of the component that scheduled the event
    int          dst;        // id of the component that should handle it
    int          packet_id;  // -1 if not used, or a packet id for DATA/ACK/TIMEOUT
    EventHandler handler;    
} Event;

extern double g_now;
extern int    g_stop_simulation;

void   event_queue_init(void);
int    event_queue_empty(void);
Event* pop_next_event(void);

void schedule_event(double time,
                    int src, int dst,
                    int packet_id,
                    EventHandler handler);

#endif

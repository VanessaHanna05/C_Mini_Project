#ifndef EVENT_H
#define EVENT_H


struct Event;                     

typedef void (*EventHandler)(struct Event *e);

typedef struct Event {
    double       time;       
    int          src;     
    int          dst;        
    int          packet_id;  
    EventHandler handler;    // pointer to a function 
} Event;

extern double g_now;
extern int    g_stop_simulation;

void   event_queue_init(void);
int    event_queue_empty(void);
Event* pop_next_event(void);

void schedule_event(double time, int src, int dst,int packet_id,EventHandler handler);

#endif

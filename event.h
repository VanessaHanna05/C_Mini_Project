#ifndef EVENT_H
#define EVENT_H
/* 
Header guards:
- EVENT_H is a unique macro name for this header file.
- #ifndef / #define ensure the compiler includes this header only once,
  even if multiple .c files include it.
*/

#include <stddef.h>
/*
We include <stddef.h> because it defines standard types like size_t and NULL.
Even if we currently only use NULL via void* pointers, this is good practice.
*/

/* 
EventType is an enumeration of all possible event kinds in this simulation.
We give EVT_SEND_SYN an explicit value of 1 so that 0 is not a valid event
(by default), and the rest auto-increment from there.
*/
typedef enum {
    EVT_SEND_SYN = 1,   // Sender wants to send the first SYN (start of handshake)
    EVT_RECV_SYN,       // Receiver "receives" the SYN through the simulated network
    EVT_SEND_SYNACK,    // (Unused in this version) would represent sending a SYNACK
    EVT_RECV_SYNACK,    // Sender receives SYNACK from receiver (handshake step 2)
    EVT_SEND_ACK,       // (Unused here) would represent sending final ACK of handshake
    EVT_RECV_ACK,       // Receiver receives ACK (handshake done, connection established)
    EVT_SEND_DATA,      // Sender wants to send a DATA packet into the network
    EVT_RECV_DATA,      // Receiver gets a DATA packet from the network
    EVT_SEND_DATA_ACK,  // (Unused directly) symbolic type for sending ACK for DATA
    EVT_RECV_DATA_ACK,  // Sender receives ACK for a DATA packet
    EVT_TIMEOUT,        // A timer expired (used here for SYN retransmission)
    EVT_RECV_FINISH     // Receiver gets a "finish" signal -> simulation stops
} EventType;

/*
Event structure:
Represents a single scheduled event in the discrete-event simulation.
Every event has:
- time : the simulation time at which the event should be processed.
- type : what kind of event (see EventType above).
- src  : id of the component that generated this event (e.g., sender id).
- dst  : id of the component that should handle this event (e.g., receiver id).
- data : optional pointer to additional data needed when handling the event.
         For example, for EVT_RECV_DATA and EVT_RECV_DATA_ACK, this is an int*
         that stores the packet id.
*/
typedef struct Event {
    double    time;   // When to process this event in simulated time.
    EventType type;   // What kind of event this is.
    int       src;    // Source component id (sender or receiver).
    int       dst;    // Destination component id.
    void     *data;   // Extra payload; interpretation depends on event type.
} Event;

/*
These globals are defined in main.c and used across several modules
(sender, receiver, etc.) to know:
- g_now            : current simulation time (updated before handling each event).
- g_stop_simulation: flag to break the main event loop when set to 1.
*/
extern double g_now;
extern int    g_stop_simulation;

/*
Public API of the event queue:

- event_queue_init():
    Initializes the global event priority queue. Must be called once before
    scheduling any events.

- schedule_event(time, type, src, dst, data):
    Creates a new Event with the given parameters and inserts it into
    the priority queue so that it will be processed at "time".

- pop_next_event():
    Removes and returns the Event with the smallest time value
    (i.e., the earliest scheduled event). Returns NULL if the queue is empty.
    The caller is responsible for freeing the returned Event after handling it.

- event_queue_empty():
    Returns non-zero (true) if there are no events left in the queue,
    zero (false) otherwise.
*/
void          schedule_event(double time, EventType type, int src, int dst, void *data);
struct Event* pop_next_event(void);
int           event_queue_empty(void);
void          event_queue_init(void);

#endif // EVENT_H

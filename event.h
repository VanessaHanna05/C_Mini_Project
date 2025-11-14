#ifndef EVENT_H
#define EVENT_H
#include <stddef.h>

/*
 * Why an Event abstraction?
 * -------------------------
 * Discrete‑event simulation advances time by “jumping” to the next scheduled event
 * instead of ticking a fixed time step. An Event packages everything we need to
 * execute at some future time (a function pointer + context + payload).
 *
 * This keeps the main loop simple and generic: pop the earliest event and run it.
 */

/* Forward-declare Event for the handler typedef so we can refer to it before defining. */
struct Event;

/*
 * EventHandler is the function signature every scheduled action must follow.
 * - ctx: points to the object that should handle the event (Sender*, Receiver*, etc.)
 * - e:   the event instance with time/src/dst/payload
 * Having the ctx argument avoids global singletons and makes unit testing easier.
 */
typedef void (*EventHandler)(void *ctx, struct Event *e);

/* Optional destructor for dynamically allocated payloads (e.g., int*).
 * Centralizing payload cleanup avoids duplication in handlers and prevents leaks.
 */
typedef void (*EventDataDtor)(void *data);

/* Core event structure carried in the priority queue. */
typedef struct Event {
    double        time;      // When this should fire (simulation time)
    int           src;       // Who scheduled/sent it (useful for logs/filters)
    int           dst;       // Intended recipient (not enforced; for tracing)
    void         *data;      // Arbitrary payload (e.g., packet id)
    EventHandler  handler;   // What to call when popped from the heap
    void         *ctx;       // Which object handles it (passed as first arg)
    EventDataDtor data_dtor; // How to free 'data' after handler runs (optional)
} Event;

/* Global clock & stop flag shared by the simulation.
 * In a larger system these could be encapsulated, but globals are pragmatic here. */
extern double g_now;
extern int    g_stop_simulation;

/*
 * schedule_event: places a new Event into the global priority queue.
 * Design choice: callers provide the exact handler+ctx rather than an enum type.
 * - Pros: no giant switch/coupling; handlers live next to their types.
 * - Cons: less static checking; relies on correct wiring at schedule time.
 */
void   schedule_event(double time, int src, int dst,
                      void *data, EventDataDtor dtor,
                      EventHandler handler, void *ctx);

/* Queue primitives used by the main loop. */
struct Event* pop_next_event(void);
int    event_queue_empty(void);
void   event_queue_init(void);

#endif

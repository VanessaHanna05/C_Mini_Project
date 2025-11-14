#ifndef HEAP_PRIORITY_H
#define HEAP_PRIORITY_H
#include "event.h"

/*
 * Minimal binary min‑heap keyed by Event->time.
 * Why a heap? It gives O(log N) insertion and O(log N) pop, which scales well
 * for simulations with many scheduled events.
 */

typedef struct {
    double time; // key for ordering — copied from Event->time to avoid deref cost in the heap logic
    Event *ev;   // pointer to the full Event to be executed
} HeapNode;

typedef struct {
    HeapNode *arr;   // dynamic array backing the heap
    int       size;  // number of valid nodes
    int       capacity; // current allocated capacity
} Heap;

/* Public API used by the rest of the simulator. */
void          event_queue_init(void);
int           event_queue_empty(void);
struct Event* pop_next_event(void);

/* Insert a new event (helper exposed so anyone can schedule). */
void schedule_event(double time,
                    int src, int dst,
                    void *data, EventDataDtor dtor,
                    EventHandler handler, void *ctx);

#endif

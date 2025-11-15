#ifndef HEAP_PRIORITY_H
#define HEAP_PRIORITY_H

#include "event.h"

/*
 * Simple binary min-heap storing Event*.
 * We no longer duplicate the event time in the heap node.
 * The heap compares events using ev->time directly.
 */

typedef struct {
    Event **arr;   // dynamic array of Event* pointers
    int     size;  // number of items currently stored
    int     cap;   // allocated capacity
} Heap;

/* Functions used by the simulator */
void event_queue_init(void);
int  event_queue_empty(void);
Event* pop_next_event(void);
void heap_insert(Event *e);

#endif

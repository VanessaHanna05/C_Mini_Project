#ifndef HEAP_PRIORITY_H //The guard makes sure the body of this header is only seen once per compilation unit.
#define HEAP_PRIORITY_H
#include "event.h"

typedef struct {
    Event **arr;  // pointer to pointer to Event so dynamic array of pointers to Event so arr[i] is of type Event*.
    // we are using double pointers in this case because heap should reorder pointers, not the events themselves (so the heap hipifies the pointers to events)
    int     size;  // Number of elements currently stored in the heap
    int     cap;  //The capacity of the heap : how many elements arr can hold before we need to realloc
} Heap;

void   event_queue_init(void);
int    event_queue_empty(void);
Event* pop_next_event(void);
void   heap_insert(Event *e);

#endif

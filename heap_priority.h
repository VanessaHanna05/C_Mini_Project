#ifndef HEAP_PRIORITY_H
#define HEAP_PRIORITY_H

#include "event.h"

typedef struct {
    double time;
    Event *ev;
} HeapNode;

typedef struct {
    HeapNode *arr;   // dynamic array of heap nodes
    int size;        // current number of elements
    int capacity;    // allocated capacity
} Heap;

void event_queue_init(void);
void schedule_event(double time, EventType type, int src, int dst, void *data);
Event *pop_next_event(void);
int event_queue_empty(void);

#endif

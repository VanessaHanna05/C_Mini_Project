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

void event_queue_init(void); //event_queue_init allocates an initial array of 100 nodes.
void schedule_event(double time, EventType type, int src, int dst, void *data); //schedule_event allocates a new Event, fills it, inserts a node at the end of the heap array, then bubble up by time. Amortized insertion cost is O(log N).
Event *pop_next_event(void); //removes the root node, moves the last node to the root, then bubble down. Removal cost is O(log N).
int event_queue_empty(void); //checks size.

#endif

#ifndef HEAP_PRIORITY_H
#define HEAP_PRIORITY_H
/*
Header guards for this priority-queue implementation.
*/

#include "event.h"
/*
We include event.h because the heap stores pointers to Event objects.
*/

/*
HeapNode:
- Represents a single node in the binary heap.
- Stores:
    time : the key used for ordering in the min-heap (same as event->time).
    ev   : pointer to the actual Event that should be executed at 'time'.
*/
typedef struct {
    double time;  // Key used to order the heap (event execution time).
    Event *ev;    // Pointer to the Event associated with this node.
} HeapNode;

/*
Heap:
- A binary min-heap implemented using a dynamic array of HeapNode.
- size     : current number of elements stored in the heap.
- capacity : how many elements the array can hold before a reallocation is needed.
- arr      : pointer to the dynamically allocated array of HeapNode.
*/
typedef struct {
    HeapNode *arr;    // Dynamic array that holds all heap nodes.
    int       size;   // How many nodes are currently in the heap.
    int       capacity; // Maximum nodes before we need to grow the array.
} Heap;

/*
Function prototypes implementing the discrete-event priority queue:

event_queue_init():
- Allocates memory for an initial heap array (e.g., capacity = 100).
- Sets size=0, meaning the queue is empty.

schedule_event(time, type, src, dst, data):
- Allocates a new Event struct on the heap.
- Fills the Event fields (time, type, src, dst, data).
- Inserts a new HeapNode containing the Event at the end of the heap array.
- Uses bubble_up to restore the min-heap property based on the 'time' key.
- Time complexity: O(log N) per insertion.

pop_next_event():
- Removes the root node (the one with minimal time, at index 0).
- Moves the last node in the array to the root position.
- Decrements size.
- Calls bubble_down to restore the heap property.
- Returns the Event* stored in the removed root node.
- Time complexity: O(log N) per removal.

event_queue_empty():
- Returns 1 if size == 0, meaning there are no events in the queue.
- Returns 0 otherwise.
*/
void   event_queue_init(void);
void   schedule_event(double time, EventType type, int src, int dst, void *data);
Event* pop_next_event(void);
int    event_queue_empty(void);

#endif // HEAP_PRIORITY_H

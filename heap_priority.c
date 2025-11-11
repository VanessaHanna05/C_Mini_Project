#include <stdio.h>      // For printf, fprintf used in debug and error output
#include <stdlib.h>     // For malloc, realloc, exit
#include <assert.h>     // For assert to check invariants
#include "heap_priority.h"  // Our own header defining Heap, HeapNode, etc.

/*
INITIAL_CAPACITY:
- Initial number of HeapNode slots we allocate in the heap array.
- This is just a starting point; we can grow the array when needed.
*/
#define INITIAL_CAPACITY 100

/* 
Global heap instance:
- We keep a single Heap structure for the whole simulation.
- It is marked static so it is only visible within this translation unit.
*/
static Heap g_heap;

/* ---------- Internal Helpers ---------- */

/*
swap_nodes:
- Swaps the contents of two HeapNode variables.
- We use this when reordering nodes during bubble_up and bubble_down.
*/
static void swap_nodes(HeapNode *a, HeapNode *b) {
    HeapNode tmp = *a;  // save a copy of *a
    *a = *b;            // copy b into a
    *b = tmp;           // copy original a into b
}

/*
bubble_up:
- Restores the min-heap property by moving the node at index i upward.
- While the current node's time is smaller than its parent's time, swap them.
- Stop when:
  * we reach the root (i == 0), or
  * the parent's time <= current node's time (heap property satisfied).
*/
static void bubble_up(Heap *h, int i) {
    while (i > 0) {                     // while not at the root
        int parent = (i - 1) / 2;       // parent index in array-based heap

        // If parent has earlier or equal time, heap property is satisfied
        if (h->arr[parent].time <= h->arr[i].time)
            break;

        // Otherwise, swap parent and child to move smaller time upward
        swap_nodes(&h->arr[parent], &h->arr[i]);

        // Continue bubbling up from the parent's new position
        i = parent;
    }
}

/*
bubble_down:
- Restores the min-heap property by moving the node at index i downward.
- At each step:
  * compute indices of left and right children,
  * find the child with the smallest 'time',
  * if that child is smaller than the current node, swap them and keep going.
- Stop when:
  * node has no children with smaller time (heap property satisfied).
*/
static void bubble_down(Heap *h, int i) {
    while (1) {
        int left     = 2 * i + 1;  // index of left child in array
        int right    = 2 * i + 2;  // index of right child
        int smallest = i;          // assume current node is smallest

        // Compare with left child if it exists and has smaller time
        if (left < h->size && h->arr[left].time < h->arr[smallest].time)
            smallest = left;

        // Compare with right child if it exists and has even smaller time
        if (right < h->size && h->arr[right].time < h->arr[smallest].time)
            smallest = right;

        // If the smallest index is still i, heap property is satisfied; stop
        if (smallest == i)
            break;

        // Otherwise, swap current node with the smallest child and continue
        swap_nodes(&h->arr[i], &h->arr[smallest]);
        i = smallest; // move down to the child's index and repeat
    }
}

/*
ensure_capacity:
- Checks if there is room for at least one more HeapNode.
- If size >= capacity, we double capacity and call realloc to grow the array.
- If realloc fails, assert will terminate the program (debug crash).
*/
static void ensure_capacity(Heap *h) {
    if (h->size >= h->capacity) {
        int newcap = h->capacity * 2;   // double the capacity
        h->arr = realloc(h->arr, newcap * sizeof(HeapNode));
        // Ensure that the reallocation succeeded; if not, abort in debug builds
        assert(h->arr && "Heap realloc failed");
        h->capacity = newcap;           // update capacity to the new value
    }
}

/* ---------- Public API ---------- */

/*
event_queue_init:
- Initializes the global heap g_heap.
- Sets:
   size = 0      (no events yet),
   capacity = INITIAL_CAPACITY (e.g. 100).
- Allocates the underlying array with malloc.
- If malloc fails, prints an error and exits the program.
*/
void event_queue_init(void) {
    g_heap.size     = 0;                  // initially no events in the queue
    g_heap.capacity = INITIAL_CAPACITY;   // starting capacity
    g_heap.arr      = malloc(g_heap.capacity * sizeof(HeapNode)); // allocate array

    // If allocation failed, print an error and exit.
    if (!g_heap.arr) {
        fprintf(stderr, "Error: failed to allocate heap array.\n");
        exit(1);
    }

    // Optional debug output to know initialization happened.
    printf("[DEBUG] event_queue_init done\n");
}

/*
schedule_event:
- Inserts a new Event into the priority queue.
- Steps:
   1. Allocate memory for a new Event object (malloc).
   2. Copy all parameters (time, type, src, dst, data) into the Event.
   3. Ensure heap has space (ensure_capacity).
   4. Insert a new HeapNode at the end of the heap array, pointing to this Event.
   5. Call bubble_up so the min-heap property holds (earliest time at root).
*/
void schedule_event(double time, EventType type, int src, int dst, void *data) {
    // 1. Allocate an Event structure on the heap (dynamic memory).
    Event *e = malloc(sizeof(Event));

    // 2. Initialize Event fields with the provided parameters.
    e->time = time;   // event will fire at "time"
    e->type = type;   // type of event (SYN, DATA, ACK, etc.)
    e->src  = src;    // who created the event
    e->dst  = dst;    // who should handle the event
    e->data = data;   // pointer to optional extra data

    // 3. Make sure the heap array has space for another node.
    ensure_capacity(&g_heap);

    // 4. Insert at the end of the heap array (index = old size).
    int i = g_heap.size++;          // i is index for the new node, then increment size
    g_heap.arr[i].time = time;      // node's key is the event time
    g_heap.arr[i].ev   = e;         // store the pointer to the Event

    // 5. Move the new node up to restore heap property.
    bubble_up(&g_heap, i);

    // Optional debug log showing when and what we just scheduled.
    printf("[DEBUG] schedule_event(time=%.3f, type=%d)\n", time, type);
}

/*
pop_next_event:
- Removes and returns the Event* with the smallest time from the heap.

Algorithm:
1. If heap is empty, return NULL.
2. Save the Event* from the root node (index 0) as result.
3. Decrease size; now last element index becomes 'size'.
4. If there are still elements:
   * move the last node to index 0,
   * call bubble_down to restore the min-heap property.
5. Return the saved Event* to the caller.

The caller is then responsible for using and freeing the Event when done.
*/
Event *pop_next_event(void) {
    // 1. If heap has no elements, nothing to return.
    if (g_heap.size == 0)
        return NULL;

    // 2. Save pointer to the event stored at the root.
    Event *e = g_heap.arr[0].ev;

    // 3. Decrease size, effectively discarding the last node (we will move it).
    g_heap.size--;

    // 4. If there are still any nodes left, move last node to root and bubble down.
    if (g_heap.size > 0) {
        g_heap.arr[0] = g_heap.arr[g_heap.size]; // move last node to root position
        bubble_down(&g_heap, 0);                 // restore heap order
    }

    // 5. Return the event pointer. The caller (main loop) will handle and free it.
    return e;
}

/*
event_queue_empty:
- Simple helper to check whether there are any events scheduled.
- Returns 1 (true) if size == 0; 0 (false) otherwise.
*/
int event_queue_empty(void) {
    return (g_heap.size == 0);
}

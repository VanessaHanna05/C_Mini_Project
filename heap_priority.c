#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "heap_priority.h"

/*
 * This module implements a global, binary min-heap of Events keyed by Event->time.
 * A min-heap lets us:
 *   - insert a new event in O(log N)
 *   - pop the earliest (smallest time) event in O(log N)
 * That’s ideal for a discrete-event simulator where the main loop always needs
 * the next chronological event.
 *
 * INDEXING MODEL (0-based):
 *   parent(i) = (i - 1) / 2
 *   left(i)   = 2*i + 1
 *   right(i)  = 2*i + 2
 *
 * INVARIANT:
 *   For every node i (except the root), heap[parent(i)].time <= heap[i].time
 *   This guarantees that the root (index 0) always holds the smallest time.
 */

/* Initial capacity keeps early allocations cheap but avoids zero-cap edge cases.
 * - If we started at 0, we'd have to special-case the first insertion.
 * - 100 is arbitrary; the heap will grow via realloc as needed. */
#define INITIAL_CAPACITY 100

/* Single global heap instance for the simulation.
 * Rationale:
 *   - The simulator typically has one event queue shared by all components.
 *   - Passing the heap around everywhere adds boilerplate with little benefit here.
 * Trade-off:
 *   - Not thread-safe by design (the whole sim is single-threaded anyway). */
static Heap g_heap;

/* ---------- internal helpers ---------- */

/* swap_nodes:
 * Swap two heap nodes in-place.
 * Why a helper?
 *   - Keeps heap maintenance code (bubble_up/down) shorter and less error-prone. */
static void swap_nodes(HeapNode *a, HeapNode *b) {
    HeapNode t = *a; *a = *b; *b = t;
}

/* bubble_up:
 * After inserting a new node at the end, we "bubble" it up until the heap
 * property is restored.
 *
 * Loop logic:
 *   - While the node is not the root (i > 0),
 *   - Compare it with its parent; if its key (time) is smaller, swap.
 *   - Move up to the parent's index and continue.
 *
 * Termination:
 *   - Either the node reaches the root, or its parent already has a smaller/equal key.
 *
 * Complexity: O(log N), because each swap moves up one level of a binary tree. */
static void bubble_up(Heap *h, int i) {
    while (i > 0) {
        int p = (i - 1) / 2;                 // parent index in a 0-based binary heap
        if (h->arr[p].time <= h->arr[i].time) break; // heap property holds; stop
        swap_nodes(&h->arr[p], &h->arr[i]);  // child is smaller: swap with parent
        i = p;                               // continue from the parent's position
    }
}

/* bubble_down:
 * After removing the root, we place the last node at the root and "sink" it
 * down until the heap property is restored.
 *
 * Loop logic:
 *   - Compute children indices (l = left, r = right).
 *   - Among the current node and its existing children, find the smallest key.
 *   - If the smallest is one of the children, swap with that child and continue.
 *   - If the current node is already smallest, stop.
 *
 * Edge cases:
 *   - A node can have 0, 1, or 2 children depending on h->size.
 *
 * Complexity: O(log N). */
static void bubble_down(Heap *h, int i) {
    for (;;) {
        int l = 2*i + 1, r = 2*i + 2, s = i; // left/right child, "s" = index of smallest so far

        // If left child exists and is smaller than current "smallest", update s.
        if (l < h->size && h->arr[l].time < h->arr[s].time) s = l;

        // If right child exists and is smaller than the current "smallest", update s again.
        if (r < h->size && h->arr[r].time < h->arr[s].time) s = r;

        if (s == i) break;                    // current node is smaller than both children: heap ok
        swap_nodes(&h->arr[i], &h->arr[s]);   // otherwise, swap with the smallest child
        i = s;                                // and continue sinking from there
    }
}

/* ensure_capacity:
 * Guarantee there's space for at least one more element.
 * Strategy:
 *   - If size == capacity, grow to either INITIAL_CAPACITY (first time)
 *     or double the capacity to amortize reallocation cost.
 *
 * Why doubling?
 *   - Amortized O(1) inserts over many push operations.
 *
 * assert:
 *   - If realloc fails, we assert(false). In a simulator this is acceptable, because
 *     without memory the run cannot proceed sensibly. */
static void ensure_capacity(Heap *h) {
    if (h->size >= h->capacity) {
        int ncap = h->capacity ? h->capacity*2 : INITIAL_CAPACITY;
        h->arr = (HeapNode*)realloc(h->arr, ncap * sizeof(HeapNode));
        assert(h->arr && "heap realloc failed"); // fail fast: without memory, the sim can't continue
        h->capacity = ncap;
    }
}

/* ---------- public API ---------- */

/* event_queue_init:
 * Initialize the global heap to an empty, ready-to-use priority queue.
 * Steps:
 *   - reset size to 0
 *   - choose an initial capacity
 *   - allocate the backing array
 * Failure handling:
 *   - If malloc fails, print an error and exit immediately. */
void event_queue_init(void) {
    g_heap.size = 0;                                        // nothing scheduled yet
    g_heap.capacity = INITIAL_CAPACITY;                     // start-sized buffer
    g_heap.arr = (HeapNode*)malloc(g_heap.capacity * sizeof(HeapNode));
    if (!g_heap.arr) { fprintf(stderr,"heap alloc failed\n"); exit(1); }
    printf("[DEBUG] event_queue_init done\n");              // optional aid while debugging
}

/* event_queue_empty:
 * Lightweight check used by the main loop to know when to stop.
 * Constant time: just compares size against 0. */
int event_queue_empty(void) {
    return g_heap.size == 0;
}

/* pop_next_event:
 * Remove and return the earliest (smallest time) Event* from the queue.
 *
 * Steps:
 *   1) If the heap is empty, return NULL — the simulator can stop or wait.
 *   2) Save the root's Event* (that's the value to return).
 *   3) Move the last node into the root position and decrease size.
 *   4) bubble_down(0) to restore the heap property.
 *   5) Return the saved Event*.
 *
 * Memory ownership:
 *   - We return the Event* but DO NOT free it here. The caller (main loop)
 *     is responsible for:
 *       a) executing the handler
 *       b) running any payload destructor (if present)
 *       c) freeing the Event object itself
 *
 * Why this split?
 *   - Decouples queue mechanics from event execution & cleanup policy, making
 *     the scheduler simpler and more reusable. */
Event* pop_next_event(void) {
    if (g_heap.size == 0) return NULL;            // no events to process

    Event *e = g_heap.arr[0].ev;                  // keep root's event to return

    g_heap.size--;                                 // effectively remove the last node
    if (g_heap.size > 0) {
        g_heap.arr[0] = g_heap.arr[g_heap.size];   // move last node to the root hole
        bubble_down(&g_heap, 0);                   // fix the heap by sinking it down
    }
    // If size became 0, we skipped bubble_down, which is correct: heap now empty.

    return e;                                      // caller owns the returned Event*
}

/* schedule_event:
 * Create a new Event and insert it into the heap at the correct position.
 *
 * Inputs:
 *   - time:     simulated time when the event should fire (key in the heap)
 *   - src/dst:  for tracing/logging (who scheduled it; who it's for)
 *   - data:     optional heap-allocated payload the handler may use (can be NULL)
 *   - dtor:     optional destructor function to free 'data' after handling
 *   - handler:  the function that should run when this event is popped
 *   - ctx:      the object that should be passed as first arg to 'handler' (like 'this')
 *
 * Steps:
 *   1) Allocate and fill an Event struct.
 *   2) Ensure heap has room; grow if needed (realloc).
 *   3) Append a new HeapNode at the end with (time, event*).
 *   4) bubble_up from that index to maintain min-heap order.
 *
 * Debug print:
 *   - Helpful when reasoning about event flow; safe to remove in production.
 *
 * Complexity:
 *   - O(log N) due to bubble_up. */
void schedule_event(double time,
                    int src, int dst,
                    void *data, EventDataDtor dtor,
                    EventHandler handler, void *ctx)
{
    /* Allocate a concrete Event object to live independently of the caller’s stack.
     * The simulator later frees this in the main loop after processing. */
    Event *e = (Event*)malloc(sizeof(Event));
    e->time      = time;     // used as the heap key
    e->src       = src;      // who scheduled this (for logs/debug)
    e->dst       = dst;      // intended recipient (informational)
    e->data      = data;     // optional payload (may be NULL)
    e->data_dtor = dtor;     // how to free 'data' after handler runs (may be NULL)
    e->handler   = handler;  // callback to execute
    e->ctx       = ctx;      // the "owner" object passed to handler

    ensure_capacity(&g_heap);         // grow backing array if we're out of room

    int i = g_heap.size++;            // index where we’ll insert the new node
    g_heap.arr[i].time = time;        // copy the key to the heap node (cheap to compare)
    g_heap.arr[i].ev   = e;           // point to the Event payload
    bubble_up(&g_heap, i);            // restore min-heap property

    printf("[DEBUG] schedule_event(time=%.3f, handler=%p)\n", time, (void*)handler);
    /* Note: If two events have the same 'time', their relative order is not stable.
     * That’s generally fine for a simulator unless strict tie-breaking is required,
     * in which case you could add a monotonic sequence number to the key. */
}

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "heap_priority.h"

#define INITIAL_CAPACITY 100

/* Global heap instance */
static Heap g_heap;

/* ---------- Internal Helpers ---------- */

static void swap_nodes(HeapNode *a, HeapNode *b) {
    HeapNode tmp = *a;
    *a = *b;
    *b = tmp;
}

/* Maintain heap property upward */
static void bubble_up(Heap *h, int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->arr[parent].time <= h->arr[i].time)
            break;
        swap_nodes(&h->arr[parent], &h->arr[i]);
        i = parent;
    }
}

/* Maintain heap property downward */
static void bubble_down(Heap *h, int i) {
    while (1) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;

        if (left < h->size && h->arr[left].time < h->arr[smallest].time)
            smallest = left;
        if (right < h->size && h->arr[right].time < h->arr[smallest].time)
            smallest = right;

        if (smallest == i)
            break;

        swap_nodes(&h->arr[i], &h->arr[smallest]);
        i = smallest;
    }
}

/* Expand heap array dynamically */
static void ensure_capacity(Heap *h) {
    if (h->size >= h->capacity) {
        int newcap = h->capacity * 2;
        h->arr = realloc(h->arr, newcap * sizeof(HeapNode));
        assert(h->arr && "Heap realloc failed");
        h->capacity = newcap;
    }
}

/* ---------- Public API ---------- */

void event_queue_init(void) {
    g_heap.size = 0;
    g_heap.capacity = INITIAL_CAPACITY;
    g_heap.arr = malloc(g_heap.capacity * sizeof(HeapNode));
    if (!g_heap.arr) {
        fprintf(stderr, "Error: failed to allocate heap array.\n");
        exit(1);
    }
    printf("[DEBUG] event_queue_init done\n");
}

/* Insert new event into heap */
void schedule_event(double time, EventType type, int src, int dst, void *data) {
    Event *e = malloc(sizeof(Event));
    e->time = time;
    e->type = type;
    e->src = src;
    e->dst = dst;
    e->data = data;

    ensure_capacity(&g_heap);

    int i = g_heap.size++;
    g_heap.arr[i].time = time;
    g_heap.arr[i].ev = e;
    bubble_up(&g_heap, i);

    printf("[DEBUG] schedule_event(time=%.3f, type=%d)\n", time, type);
}

/* Remove and return event with minimum time */
Event *pop_next_event(void) {
    if (g_heap.size == 0)
        return NULL;

    Event *e = g_heap.arr[0].ev;
    g_heap.size--;

    if (g_heap.size > 0) {
        g_heap.arr[0] = g_heap.arr[g_heap.size];
        bubble_down(&g_heap, 0);
    }

    return e;
}

/* Return whether heap is empty */
int event_queue_empty(void) {
    return (g_heap.size == 0);
}

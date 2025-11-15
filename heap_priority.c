#include <stdio.h>
#include <stdlib.h>
#include "heap_priority.h"

/* Single global heap instance used by the whole simulator. */
static Heap g_heap;

/* Swap two Event* pointers in the heap array. */
static void swap(Event **a, Event **b) {
    Event *tmp = *a;
    *a = *b;
    *b = tmp;
}

/* Ensure the heap array has at least new_cap capacity. */
static void ensure_capacity(Heap *h, int new_cap) {
    if (h->cap >= new_cap) return;

    int new_capacity = (h->cap == 0) ? 16 : h->cap * 2;
    if (new_capacity < new_cap) new_capacity = new_cap;

    Event **new_arr = realloc(h->arr, new_capacity * sizeof(Event*));
    if (!new_arr) {
        fprintf(stderr, "Heap allocation failed\n");
        exit(1);
    }

    h->arr = new_arr;
    h->cap = new_capacity;
}

/* Move node at index i upwards until heap property (min at root) is satisfied. */
static void bubble_up(Heap *h, int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;

        /* Compare using e->time directly; no time stored in heap node. */
        if (h->arr[parent]->time <= h->arr[i]->time)
            break;

        swap(&h->arr[parent], &h->arr[i]);
        i = parent;
    }
}

/* Move node at index i downwards until heap property is satisfied. */
static void bubble_down(Heap *h, int i) {
    while (1) {
        int left  = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;

        if (left < h->size &&
            h->arr[left]->time < h->arr[smallest]->time)
            smallest = left;

        if (right < h->size &&
            h->arr[right]->time < h->arr[smallest]->time)
            smallest = right;

        if (smallest == i) break;

        swap(&h->arr[i], &h->arr[smallest]);
        i = smallest;
    }
}

/* Insert a new event pointer into the heap. */
void heap_insert(Event *e) {
    ensure_capacity(&g_heap, g_heap.size + 1);
    g_heap.arr[g_heap.size] = e;
    bubble_up(&g_heap, g_heap.size);
    g_heap.size++;
}

/* Initialize the global event queue. */
void event_queue_init(void) {
    g_heap.arr  = NULL;
    g_heap.cap  = 0;
    g_heap.size = 0;
}

/* Returns 1 if queue is empty, 0 otherwise. */
int event_queue_empty(void) {
    return g_heap.size == 0;
}

/* Remove and return the earliest event (smallest time) from the heap. */
Event* pop_next_event(void) {
    if (g_heap.size == 0) return NULL;

    Event *ev = g_heap.arr[0];

    /* Move last element to root and shrink. */
    g_heap.arr[0] = g_heap.arr[g_heap.size - 1];
    g_heap.size--;

    if (g_heap.size > 0)
        bubble_down(&g_heap, 0);

    return ev;
}

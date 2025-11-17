#include <stdio.h>
#include <stdlib.h>
#include "heap_priority.h"

static Heap g_heap;

static void swap(Event **a, Event **b) {
    Event *tmp = *a;
    *a = *b;
    *b = tmp;
}

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

static void bubble_up(Heap *h, int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->arr[parent]->time <= h->arr[i]->time)
            break;

        swap(&h->arr[parent], &h->arr[i]);
        i = parent;
    }
}

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

void heap_insert(Event *e) {
    ensure_capacity(&g_heap, g_heap.size + 1);
    g_heap.arr[g_heap.size] = e;
    bubble_up(&g_heap, g_heap.size);
    g_heap.size++;
}

void event_queue_init(void) {
    g_heap.arr  = NULL;
    g_heap.cap  = 0;
    g_heap.size = 0;
}

int event_queue_empty(void) {
    return g_heap.size == 0;
}

Event* pop_next_event(void) {
    if (g_heap.size == 0) return NULL;

    Event *ev = g_heap.arr[0];

    g_heap.arr[0] = g_heap.arr[g_heap.size - 1];
    g_heap.size--;

    if (g_heap.size > 0)
        bubble_down(&g_heap, 0);

    return ev;
}

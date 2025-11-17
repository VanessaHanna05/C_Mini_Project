#include <stdio.h>
#include <stdlib.h>
#include "heap_priority.h"


/*
Inside the heap to be able to manipulate children and parents we can use these indexes
parent(i) = (i - 1) / 2
left_child(i) = 2*i + 1
right_child(i) = 2*i + 2
*/

Heap g_heap;

//to move one element up or down we need to swap the pointer to the event 
// **a and **b are pointers to the pointers to the events and we are swapping pointers to the events 
static void swap(Event **a, Event **b) {
    Event *tmp = *a;
    *a = *b;
    *b = tmp;
}
// this function check everytime before we insert an event inside the heap that there is still space (size < cap) otherwise is will reallocate memory 
static void ensure_capacity(Heap *h, int new_cap) {
    if (h->cap >= new_cap) return; // check if there is still space, return and do nothing

    int new_capacity =  h->cap * 2; // oterhwise double the capacity 
    if (new_capacity < new_cap) new_capacity = new_cap; 

    /*
    how to use realloc: 
    void *realloc(void *ptr, size_t new_size);
    ptr is pointer to the previously allocated memory
    new_size is how many bytes the new memory block should have
    If the block can grow in-place, it does;
    otherwise, it allocates a new block, copies the old data, and frees the old block.
*/
    Event **new_arr = realloc(h->arr, new_capacity * sizeof(Event*));
    //new_capacity * sizeof(Event*) It is the size of a the array of pointers to Event.
    h->arr = new_arr;
    h->cap = new_capacity;
}

//after inserting an element at the bottom of the tree we need to move it to the right location: 
//parent->time <= children->time
// i starts as the size of the heap because we are inserting the event at the end of the tree 

static void bubble_up(Heap *h, int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        //h->arr[parent]->time we are reading the value of the time 
        if (h->arr[parent]->time <= h->arr[i]->time)
            break; // do nothing if the time of the parent is smaller than the time of the child
        //&h->arr[parent] is the address of the array where there is the element of index parent 
        swap(&h->arr[parent], &h->arr[i]); // otherwise if the location is false we swap them using the pointers to the events (we swap based on the address of the array where we can access the address of the events that we are swapping)
        i = parent;
    }
}

/*After removing the root element: 
Save the event at index 0 and replace it with the last element 
arr[0] = arr.[size-1]
Decrease size
Fix the heap by bubbling the new root DOWN*/
// we always start with i = 0 because when we remove an event  g_heap.arr[0] = g_heap.arr[g_heap.size - 1] -> bubble_down(&g_heap, 0)

static void bubble_down(Heap *h, int i) {
    while (1) { // we need to go through the whole tree to make sure that all the elements are reordered
        int left  = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;
        if (left < h->size && h->arr[left]->time < h->arr[smallest]->time)
            smallest = left;
        if (right < h->size && h->arr[right]->time < h->arr[smallest]->time)
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
    Event *recent_event = g_heap.arr[0];

    g_heap.arr[0] = g_heap.arr[g_heap.size - 1];
    g_heap.size--;
    if (g_heap.size > 0)
        bubble_down(&g_heap, 0);

    return recent_event;
}

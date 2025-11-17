#ifndef HEAP_PRIORITY_H
#define HEAP_PRIORITY_H

#include "event.h"

typedef struct {
    Event **arr;  
    int     size;  
    int     cap;  
} Heap;

void   event_queue_init(void);
int    event_queue_empty(void);
Event* pop_next_event(void);
void   heap_insert(Event *e);

#endif

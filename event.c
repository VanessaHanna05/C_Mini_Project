#include <stdlib.h>
#include <stdio.h>
#include "event.h"
#include "heap_priority.h"


double g_now = 0.0;
int    g_stop_simulation = 0;

void schedule_event(double time, int src, int dst, int packet_id, EventHandler handler)
{
    Event *e = (Event*)malloc(sizeof(Event));
    if (!e) {
        fprintf(stderr, "Failed to allocate Event\n");
        exit(1);
    }

    e->time      = time;
    e->src       = src;
    e->dst       = dst;
    e->packet_id = packet_id;
    e->handler   = handler;

    heap_insert(e);
}

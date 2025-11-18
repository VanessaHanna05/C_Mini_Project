#include <stdlib.h>
#include <stdio.h>
#include "event.h"
#include "heap_priority.h"
double g_now = 0.0;
int    g_stop_simulation = 0;

void schedule_event(double time, int src, int dst, int packet_id, EventHandler handler)
{
    // creating a block of memory for an event -> return the address of that memory -> treat it as an Event*, and store it in recent_event
    Event *recent_event = (Event*)malloc(sizeof(Event));
    recent_event->time      = time;
    recent_event->src       = src;
    recent_event->dst       = dst;
    recent_event->packet_id = packet_id;
    recent_event->handler   = handler;

    heap_insert(recent_event);
}

#ifndef SENDER_H
#define SENDER_H
/*
Header guard for Sender module.
*/

#include "event.h"   // For EventType, Event, global g_now declaration
#include "network.h" // For Network type used in sender_handle

/*
MAX_PKTS:
- Maximum number of packets whose ACK status we track.
- This defines the length of the acked[] array in Sender.
*/
#define MAX_PKTS 10000

/*
Sender:
- Represents the sending endpoint in the simulation.

Fields:
- id                 : unique id of the sender.
- sent               : counts how many DATA packets were actually scheduled
                       through the network (i.e., not "lost before schedule").
- lost_events_counter: counts how many DATA packets we dropped *before* scheduling
                       (simulating loss at the sender side).
- empty_counter      : reserved for tracking empty packets (not used in this code).
- drop_counter       : reserved for tracking drops at queue or network (unused here).
- next_send_time     : next planned send time (not heavily used; send times are
                       computed on the fly using g_now + send_interval).
- send_interval      : amount of time between two consecutive DATA sends.
- duration           : total duration for which the sender keeps sending DATA.
- next_pkt_id        : id assigned to the next DATA packet to send.
- acked[MAX_PKTS]    : array of flags indicating whether each packet id has been
                       acknowledged (1 = ACK received, 0 = not yet).
*/
typedef struct {
    int    id;                  // Sender's id (e.g., 0).
    int    sent;                // Successfully scheduled DATA packets.
    int    lost_events_counter; // Packets "lost" before they enter the queue.
    int    empty_counter;       // Reserved counter (unused).
    int    drop_counter;        // Reserved counter (unused).
    double next_send_time;      // Planned time for the next send.
    double send_interval;       // Interval between DATA sends in seconds.
    double duration;            // Total simulation time to keep sending.
    int    next_pkt_id;         // Id to assign to the next DATA packet.
    char   acked[MAX_PKTS];     // ACK status for each packet id (0/1).
} Sender;

/*
sender_init:
- Initializes a Sender structure and schedules the very first events:
  * EVT_SEND_SYN at time 0.0 to start the handshake.
  * EVT_TIMEOUT at time SYN_RTO for the initial SYN.

Parameters:
- s               : pointer to Sender structure to initialize.
- id              : sender's id (distinguishes from receiver).
- send_interval_s : time in seconds between consecutive DATA sends.
- duration_s      : total simulation duration for sending.
*/
void sender_init(Sender *s, int id, double send_interval_s, double duration_s);

/*
sender_handle:
- Main handler for events that belong to the sender side.

Parameters:
- s   : pointer to Sender state.
- net : pointer to Network model for scheduling deliveries.
- e   : pointer to Event popped from the event queue.

Handled event types:
- EVT_SEND_SYN     : send SYN to receiver.
- EVT_RECV_SYNACK  : receive SYNACK, mark handshake complete, send ACK, start data.
- EVT_SEND_DATA    : send DATA packet (possibly lost before scheduling).
- EVT_RECV_DATA_ACK: receive ACK for DATA, mark acked.
- EVT_TIMEOUT      : timeout for SYN (if not acked, retransmit).
*/
void sender_handle(Sender *s, Network* net, Event *e);

#endif // SENDER_H

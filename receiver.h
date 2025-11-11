#ifndef RECEIVER_H
#define RECEIVER_H
/*
Header guard for the Receiver module.
*/

#include "event.h"   // For EventType, Event struct
#include "network.h" // For Network type used in receiver_handle

/*
Receiver:
- Represents the receiving endpoint of the simulated communication.

Fields:
- id              : unique identifier for the receiver (used in src/dst of events).
- received_ok     : counts valid DATA packets that were successfully received.
- invalid_packets : counter reserved for malformed or unexpected packets
                    (not used meaningfully in this version but left for extension).
*/
typedef struct {
    int id;              // Unique id of this receiver (e.g., 1).
    int received_ok;     // Counter of valid DATA packets received and processed.
    int invalid_packets; // Counter of invalid or unexpected packets (unused here).
} Receiver;

/*
receiver_init:
- Initializes a Receiver structure before it is used.
- Sets:
    * id to the provided value,
    * received_ok and invalid_packets to 0.
*/
void receiver_init(Receiver *r, int id);

/*
receiver_handle:
- Main dispatcher for receiver-side events.
- It is called from the main loop for any event whose type is handled by
  the receiver (EVT_RECV_SYN, EVT_RECV_ACK, EVT_RECV_DATA, EVT_RECV_FINISH).
- Parameters:
    r   : pointer to Receiver state.
    net : pointer to Network model, used to send responses back (e.g., ACK).
    e   : pointer to the Event received from the event queue.
*/
void receiver_handle(Receiver *r, Network* net, Event *e);

#endif // RECEIVER_H

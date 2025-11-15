#ifndef GLOBALS_H
#define GLOBALS_H

/* Forward declarations so we can reference these types. */
struct Network;
struct Sender;
struct Receiver;

/*
 * We keep one global instance of each main component:
 *   - g_net     : the network model (delays/jitter)
 *   - g_sender  : the sending endpoint
 *   - g_receiver: the receiving endpoint
 *
 * Handlers include this header and access these variables directly.
 * This is easier than passing context pointers around for a small project.
 */
extern struct Network  g_net;
extern struct Sender   g_sender;
extern struct Receiver g_receiver;

#endif

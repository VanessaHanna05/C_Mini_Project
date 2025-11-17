#ifndef GLOBALS_H
#define GLOBALS_H

//Reference: https://www.geeksforgeeks.org/c/understanding-extern-keyword-in-c/
/*NOTE: 
We must include the forward declarations (struct Network;)
 so the compiler knows the types exist before we declare the 
 global extern struct Network g_net;, otherwise it doesn’t know what 
 struct Network means.*/

//forward declaration because we are using struct Network as a type in the next extern declaration
struct Network;
struct Sender;
struct Receiver;

extern struct Network  g_net;
extern struct Sender   g_sender;
extern struct Receiver g_receiver;
extern double g_now;

#endif

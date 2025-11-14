#ifndef GLOBALS_H
#define GLOBALS_H

/* 
   globals.h should NOT redefine structs that already exist elsewhere.
   It only declares extern pointers to them. 
   So we use "struct Network" (not typedef).
*/

struct Network;
struct Sender;
struct Receiver;
// Question why do we need the pointer 
/* Global instance pointers (defined in main.c) */
extern struct Network  *G_NET;
extern struct Sender   *G_SND;
extern struct Receiver *G_RCV;

#endif

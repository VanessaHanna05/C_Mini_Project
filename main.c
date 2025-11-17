#include <stdio.h>
//Provides I/O functions like printf, fprintf.
#include <stdlib.h>
//Provides functions like malloc, free, atof, rand, srand, and exit
#include <time.h>
//Provides time() to get the current time for seeding the random generator.
#include "event.h"
#include "heap_priority.h"
#include "network.h"
#include "sender.h"
#include "receiver.h"
#include "globals.h"

//NOTE: ALL THE VARIABLES THAT HAVE A g AS A PREFIX ARE GLOBAL VARIABLES 
// #include tells the preprocessor to copy the contents of a header file into this file before compilation
//These are global variables used in all the program 
Network  g_net;
Sender   g_sender;
Receiver g_receiver;
// the main function takes as arguments the file name to be ran, the interval duration, the full duration 
int main(int argc, char **argv) {
    //these are default variables in case we do not specify them in the terminal 
    double send_interval = 0.05;  // the interval between each packets 
    double duration      = 1.0;   // The full duration of sending packets 
    double base_delay    = 0.05;   // the based delay that is the average delay of a network (in this case I increased the delay to see the effect of timeout )
    double jitter        = 0.2;   // maximum variation around the base delay.

    //atof is a function that takes a string (ascii) and converts it into a float 
    // Reference: https://www.w3schools.com/c/ref_stdlib_atof.php#:~:text=Definition%20and%20Usage,not%20part%20of%20the%20number.
    if (argc == 3 || argc == 5) {
        send_interval = atof(argv[1]);
        duration      = atof(argv[2]);
        if (argc == 5) {
            base_delay = atof(argv[3]);
            jitter     = atof(argv[4]);
        }
    } 
    // srand is a function that changes the numerique sequence of generating random numbers using rand in the other 
    //portions of the code. it takes tim(0) which is each second the current time that changes and thats why everytime 
    //we run the code we will have a difference random number 
    // Reference: https://www.w3schools.com/c/c_random_numbers.php
    srand((unsigned)time(0));

    event_queue_init();
    network_init(&g_net, base_delay, jitter);
    receiver_init(&g_receiver, RECEIVER_ID);
    sender_init(&g_sender,  SENDER_ID, send_interval, duration);
    // this is the main part of the code where as long as we have a scheduled event in the queue we keep popping the top event
    // everytime we pop the event we rehypify the tree and reorder the elements using a simple swap method 
    // this loop not only does it stop when the queue is empty but also we need the full duration to be over 

    while (!g_stop_simulation && !event_queue_empty()) {
        Event *recent_event = pop_next_event(); // we remove the event from the heap tree and returns the pointer to the most recent event (the root of the tree)
        g_now = recent_event->time;     // we update the current time by assigning it the value of the time of the event we popped to discretize time   
        recent_event->handler(recent_event);       //  handler is of type EventHandler, a function pointer so it points to the function that should handle the event 
        free(recent_event);               //The event was allocated with malloc when scheduled -> once the event is popped we remove it from the heap and free the memory 
    }

    // once we exit the while loop it means that the simulation is over 
printf("\nSimulation finished\n");
//g_sender.sent is a variable incremented whenever the sender actually pushes a packet into the network.
//g_sender.lost_local is incremented when the sender randomly drops packet before sending.
int logical_packets = g_sender.sent + g_sender.lost_local; //total number of data packets the sender intended to send = actually sent to network + locally dropped.
int unique_ok       = g_receiver.unique_ok; //from Receiver, counts how many different packets were successfully received without duplicates invremented in the receiver
int total_delivs    = g_receiver.received_ok; // total number of packets accepted at receiver including duplicates.
int retransmissions = total_delivs - unique_ok; // number of duplicated deliveries due to retransmissions.

printf("Sender logical packets   : %d\n", logical_packets);
printf("  - scheduled to network : %d\n", g_sender.sent);
printf("  - local drops          : %d\n", g_sender.lost_local);

printf("Receiver unique packets  : %d\n", unique_ok);
printf("Receiver total deliveries: %d\n", total_delivs);
printf("Retransmissions received (due to timeouts) : %d\n", retransmissions);
printf("Receiver invalid packets : %d\n", g_receiver.invalid_packets);


double avg_delay = (g_net.sum_delay / g_net.count_delay); // calculating the average delay of the network the count delay is incremented everytime a packet is passed to the network 

printf("Average one-way network delay: %.6f s (from %d deliveries)\n", avg_delay, g_net.count_delay);
}

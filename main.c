#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "event.h"
#include "heap_priority.h"
#include "network.h"
#include "sender.h"
#include "receiver.h"


double g_now = 0.0; // current simulation time it is updated everytime we pop an event and by adding the events timings
int g_stop_simulation = 0; // flag to stop the simulation that is set to 1 when the sender sends finish 

/* The main function takes the arguments of the user that are specified in the command line. 
The user should provide 3 arguments, the file to simulate, the interval between the messages sent, and the full time to send (= when to stop)
**arg is used as a pointer to an array of pointer in which each argument is saved arg[0] is the file to simulate, arg[1] is the interval, arg[2] is the full duration*/
int main(int argc, char **argv) {
    if (argc < 3) {
        /*if the user specifies less than 3 characters we display a message in the terminal showcasing how the message should be written, another design approach 
        would be have a default value of each argument
        Note: We usually use stdout to output a message in the terminal, however to distinguish between the normal outputs and error messages 
        we use stderr but if we use stdout is would do the same and write in the terminal */
        fprintf(stderr, "Error: missing arguments. Got %d, need at least 3\n", argc);
        fprintf(stderr, "Run as: %s interval_ms duration_s\n", argv[0]);

        return 1;
        // if the user does not specify the three arguments we will exit the main program and not continue witj the usual execution
    }

    double send_interval_ms = atof(argv[1]);
    // these are the intervals extracted from the terminal inputs 
    // atof : ascii to float converts the strings into float numbers because in memory these characters are saved as strings from the terminal 
    double duration_s       = atof(argv[2]);
    // this is the full duration of the simulation, after we cross this duration, the simulation stops 
    if (send_interval_ms <= 0 || duration_s <= 0) {
        // if any of the arguments are invalid (negative inputs or zero durations and intervals) there is an error and we exit 
        fprintf(stderr, "Invalid args: interval_ms and duration_s must be > 0\n");
        return 1;
    }
    double send_interval_s = send_interval_ms / 1000.0;
    // since we are using everything in seconds (SI units)

    srand((unsigned)time(0));
    // srand in a function that takes usually a seed number that sets the starting point of the sequence. 
    // srand is used to make sure that the starting point of the random generators used in the code later do not start always with the same numbers at each run
    // reference: https://www.geeksforgeeks.org/cpp/rand-and-srand-in-ccpp/
  
    // initiate instances of each party of the network
    Network net;
    Sender  snd;
    Receiver rcv;

    // initializing the parties
    // The netwok takes the pointer to the network structure that contains: the based delay and the jitter 
    // the arguments of the structure are used to calculate a random delay each time withing the range: [0.00, 0.02) seconds
    // the delay uses the formula: double d = base_delay + (2.0 * frand01() - 1.0) * jitter;
    //frand is a function that generates a random number between 0 and 1
    // the second argument is the based delay: by choice 
    // the third argument is the error around the delay so based delay +- jitter 
    // why choose this range for the delay: https://www.reddit.com/r/HomeNetworking/comments/v685oq/what_should_your_ping_to_your_wifi_router_be_under/#:~:text=Exotic
    network_init(&net,  0.01,  0.01);

    /* the event_queue_init(void) is a function that is used only to initialize the discrete event queue that is used to schedule events
    This function creates the queue without scheduling anything
    The start size of the queue is 0 and is empty 
    The max size of the queue is set to 100 
    Allocate memory to the queue of 100 elements first and the size of the queue chnages based on the number of events through reallocations 
    The allocation of memory is based on the number of elements in the queue and the size of the heapnode that contains the event and the time of scheduling the event 
    */
    event_queue_init();

    /*The sender_init is used to initilize the sender structure that contains:
    int id;
    int sent; Counts successfully scheduled DATA events
    int lost_events_counter; Counts how many packets were "lost" before sending (frand)
    double next_send_time;
    double send_interval; How often to schedule EVT_SEND_DATA
    double duration; When to stop sending data
    int next_pkt_id;
    char *acked; 
    and it is used to schedule the first event that is send a syn 
    The sender id is set to 0 to distinguish between the sender and the receiver packets 
    The sender schedules after this event a timeout to start a timer of 0.5s this timer if passed 0.5s will generate w timeout
    The arguments to this function are the interval in seconds and the final duration in seconds
    */
    sender_init(&snd, 0, send_interval_s, duration_s);

    /*
    the receiver_initializes the receiver structure that contains:   
    r->id = id;
    r->received_ok = 0; counter of successfully received packets 
    r->invalid_packets = 0; Initializes a counter for bad/malformed/unexpected packets
    The first argument of the function is a pointer to the receiver structure and the second is the receiver ID
    */
    receiver_init(&rcv,  1);

    /* main simulation loop */
    // keep polling from the queue as long as there are no more events there and as long as the the sender did not send a finish
    while (!event_queue_empty() && !g_stop_simulation) {
        Event* e = pop_next_event(); // extract the next scheduled event, and move the simulation clock forward to that time.
        if (!e) break; // if invalid or empty event 
        g_now = e->time;

        switch (e->type) {
        /* sender-owned events */
        case EVT_SEND_SYN:
        /*
        Sender is initiating the 3-way handshake (like TCP) 
        Starts the connection setup with the receiver 
        Schedules EVT_RECV_SYN at the receiver 
        */
        case EVT_SEND_DATA:
        /*
        Time to send a data packet
        Keeps simulation moving forward; simulates continuous sending
        Schedules EVT_RECV_DATA for receiver (maybe with loss)
        Schedules next SEND_DATA (loop until end)
        */
        case EVT_RECV_SYNACK:
        /*
        Sender gets confirmation from receiver
        Connection established, begin sending data
        Sends ACK, and schedules first DATA packet
        */
        case EVT_RECV_DATA_ACK:
        /*
        Sender got an ACK for a data packet
        Confirms successful deliver
        Marks the packet as acked in sender’s array
        */
        case EVT_TIMEOUT:
        /*
        Something timed out 
        Packet might be lost; retry the SYN or DATA
        Sender checks if ACK was received. If not, resends SYN 
        */
            sender_handle(&snd, &net, e);
            break;

        /* receiver-owned events */
        case EVT_RECV_SYN:
        /*
        Receiver gets the SYN packet
        It responds with a SYNACK to the sender
        Schedules EVT_RECV_SYNACK back to sender
        */
        case EVT_RECV_ACK:
        /*
        Receiver gets final handshake ACK
        Final step in handshake — now ready to accept data
        Nothing extra, just a log and acknowledgment
        */
        case EVT_RECV_DATA:
        /*
        Receiver got a data packet
        Receiver logs it and sends an ACK
        Schedules EVT_RECV_DATA_ACK back to sender
        */
        case EVT_RECV_FINISH:
        /*
        Simulation is done — all data has been sent
        Sets g_stop_simulation = 1, and prints done!
        */
            receiver_handle(&rcv, &net, e);
            break; 

        default:
            break;
        }

        // it is very important to remove the handled events from memory so we dont get memory overload 
        free(e);
    }

    printf("\nSimulation finished\n");
    printf("Sender: sent_scheduled=%d, lost_events=%d, empty_pkts=%d, data_drops=%d\n",
       snd.sent, snd.lost_events_counter, snd.empty_counter, snd.drop_counter);
    printf("Receiver: ok=%d, invalid=%d\n", rcv.received_ok, rcv.invalid_packets);

    return 0;
}

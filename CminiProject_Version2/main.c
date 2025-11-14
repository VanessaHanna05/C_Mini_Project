#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "event.h"
#include "heap_priority.h"
#include "network.h"
#include "sender.h"
#include "receiver.h"
#include "globals.h"  // declares G_NET/G_SND/G_RCV used by handlers

/*
 * -----------------------------------------------------------------------------
 *  PURPOSE OF THIS FILE
 * -----------------------------------------------------------------------------
 *  This is the simulation "orchestrator":
 *    - parses command-line arguments (or uses friendly defaults)
 *    - seeds randomness (affects jitter and local-loss randomness)
 *    - initializes the global priority queue for events
 *    - constructs the actors (Network, Sender, Receiver) and wires pointers
 *    - runs the main discrete-event loop:
 *        pop the earliest event -> set g_now -> call its handler -> clean up
 *    - prints a compact end-of-run summary
 *
 *  Design philosophy:
 *    Keep main() small and declarative. The protocol details (handshake, data,
 *    timeouts) are *owned by the components themselves* and expressed as
 *    scheduled events rather than hard-coded branching here.
 * -----------------------------------------------------------------------------
 */

/* Global simulation time & stop flag; simple and adequate for a single-threaded sim.
 *  - g_now is the single source of truth for "current time" inside handlers.
 *  - g_stop_simulation lets any component request an orderly shutdown without
 *    aborting the process (so we can still print stats, free memory, etc.). */
double g_now = 0.0;
int    g_stop_simulation = 0;

/* Pointers to the single instances so handlers can reach peers without globals
 * inside their modules.
 *
 * Why not pass everything through Event->ctx?
 *   We do pass each handler its *own* context via Event->ctx, but cross-component
 *   scheduling (receiver scheduling a callback into sender, or vice versa) still
 *   needs a reference to the *other* endpoint. Exposing these three pointers keeps
 *   the wiring simple and avoids building a larger service-locator layer. */
Network  *G_NET = NULL;
Sender   *G_SND = NULL;
Receiver *G_RCV = NULL;

int main(int argc, char **argv){
    // Friendly defaults so the program "just works" when run with no args.
    // These values are intentionally small for fast, visually rich traces.
    double send_interval = 0.05;  // 50ms between packets (emission cadence)
    double duration      = 1.00;  // stop sending at t = 1.00s (absolute sim time)
    double base_delay    = 0.01;  // 10ms one-way network delay (propagation+tx)
    double jitter        = 0.01;  // ±10ms variability (uniform)

    /* CLI parsing:
     *  - Allow (2) or (4) numeric args; otherwise fall back to defaults.
     *  - This gives users a quick way to experiment with timing knobs without editing code.
     *
     *    Usage examples:
     *      ./sim                 -> uses defaults above
     *      ./sim 0.02 2.0        -> 20ms send interval, run for 2.0s
     *      ./sim 0.02 2.0 0.005 0.010 -> tweak base delay & jitter as well
     *
     * Implication:
     *  - We deliberately *don’t* validate for negative or silly values here; the
     *    components handle edge cases defensively. If you want stricter UX,
     *    add checks and error messages here. */
    if (argc == 3 || argc == 5) {
        send_interval = atof(argv[1]);
        duration      = atof(argv[2]);
        if (argc == 5) {
            base_delay = atof(argv[3]);
            jitter     = atof(argv[4]);
        }
    } else if (argc != 1) {
        fprintf(stderr,
            "Usage:\n"
            "  %s                      (use defaults)\n"
            "  %s <send_interval> <duration>\n"
            "  %s <send_interval> <duration> <base_delay> <jitter>\n",
            argv[0], argv[0], argv[0]);
        return 1; // Exit early on malformed arg count (keeps behavior explicit).
    }

    /* Seed RNG so losses/jitter vary per run.
     *  - Using time(NULL) is sufficient for demos.
     *  - If you want reproducible runs, replace with a fixed seed or a CLI flag. */
    srand((unsigned)time(NULL));

    /* Build the event queue before any scheduling happens.
     *  - The queue is a min-heap ordered by Event->time.
     *  - All schedule_event() calls later rely on this being ready. */
    event_queue_init();

    /* Stack-allocate the three actors (lifetime: whole main scope).
     *  - No heap here means simple lifetime management; everything dies with main().
     *  - If you wanted to dynamically create/destroy links or endpoints during
     *    the sim, you’d move these to heap allocations and add cleanup paths. */
    Network  net;
    Sender   snd;
    Receiver rcv;

    /* Wire globals so handlers can find peers easily without complex plumbing.
     *  - These global pointers are the minimal “service locator” our handlers use
     *    when scheduling cross-component events (e.g., receiver -> sender). */
    G_NET = &net;
    G_SND = &snd;
    G_RCV = &rcv;

    /* Initialize actors.
     *  Order matters (slightly):
     *   - Receiver first: so it’s ready to accept the SYN.
     *   - Sender last: sender_init() immediately schedules the first SYN & timeout. */
    network_init(&net, base_delay, jitter);
    receiver_init(&rcv, /*id*/1);
    sender_init(&snd,   /*id*/0, send_interval, duration);

    /* ------------------------------ MAIN LOOP ------------------------------
     * The core of a discrete-event simulator:
     *   while (queue not empty && not told to stop):
     *       e = pop earliest event
     *       g_now = e->time         (time jump!)
     *       e->handler(e->ctx, e)   (execute it)
     *       cleanup payload + event
     *
     * Why this is powerful:
     *   The sim doesn’t “tick” through every microsecond. Time jumps to exactly
     *   when something happens, which makes it efficient and conceptually clean. */
    while (!g_stop_simulation && !event_queue_empty()) {
        Event *e = pop_next_event();
        if (!e) break;                  // defensive: heap API guarantees NULL only if empty

        g_now = e->time;                // establish the current simulated time

        e->handler(e->ctx, e);          // run the event’s function against its target object

        /* Centralized payload cleanup:
         *  - Handlers don’t need to remember to free their payloads on every path.
         *  - If e->data was heap-allocated and a destructor was provided when the
         *    event was scheduled, we free it here (after the handler finishes).
         *  - This pattern is robust against early returns inside handlers. */
        if (e->data && e->data_dtor) {
            e->data_dtor(e->data);
        }

        free(e);                        // Always free the Event wrapper itself.
    }

    /* End-of-run summary:
     *  - Simple counters help confirm expectations and are useful for quick
     *    regressions when changing parameters or code.
     *  - We also report an average one-way delay as observed by the network model. */
    printf("\n=== Stats ===\n");
    printf("Attempted sends: %d\n", snd.sent + snd.lost_events_counter);
    printf("Sender sent: %d\n",        snd.sent);
    printf("Receiver ok: %d\n",        rcv.received_ok);
    printf("Dropped before schedule: %d\n", snd.lost_events_counter);

    double avg_delay = (net.count_delay > 0) ? (net.sum_delay / net.count_delay) : 0.0;
    printf("Avg one-way network delay: %.6f s (from %d deliveries)\n",
       avg_delay, net.count_delay);

    /* Why return 0?
     *  - Conventional success exit; lets shell scripts or test harnesses know the
     *    sim completed normally. Non-zero would indicate argument/alloc/logic issues. */
    return 0;
}

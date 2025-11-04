# Simple Discrete-Event Simulator (INF530 – Subject 1)

This project is a **mini discrete-event simulation** of a sender–network–receiver pipeline:
- **Sender** performs a 3-way handshake (SYN / SYNACK / ACK), then sends data periodically.
- **Network** injects random delay and delivers events.
- **Receiver** is **event-driven** and remains idle between events. It updates counters and returns ACKs.

The **event queue** is a binary min-heap (`heap_priority.c`) ordered by the event timestamp.  
The **simulation clock** jumps to the next event time; no real time is used.

---

## Features

- CLI-configurable **send interval** and **duration**:
  - `./sim 100 20` → send every **100 ms** for **20 s** (simulated time).
- **Finish event**: when sender reaches the duration, it notifies receiver with `EVT_RECV_FINISH` → receiver prints `done!` and **stops** the simulation.
- **Empty packets**: sender occasionally sends **empty packets** (low probability). Receiver counts them as **invalid** and **does not ACK**.
- **Lost before scheduling**: sender sometimes “sends” but **does not schedule** the event (low probability). Sender increments **lost_events_counter**.
- **ACK timeout** for DATA: if no ACK arrives before timeout → **drop** (no retransmit). Counted in **drop_counter**.
- **SYNACK timeout**: sender **retransmits SYN** until handshake completes.
- **Reference**

---

## Build

```bash
make
./sim 20 10



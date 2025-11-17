CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11

OBJS = main.o event.o heap_priority.o sender.o receiver.o network.o

all: sim

sim: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c event.h heap_priority.h sender.h receiver.h network.h globals.h
	$(CC) $(CFLAGS) -c main.c

event.o: event.c event.h heap_priority.h
	$(CC) $(CFLAGS) -c event.c

heap_priority.o: heap_priority.c heap_priority.h event.h
	$(CC) $(CFLAGS) -c heap_priority.c

sender.o: sender.c sender.h event.h heap_priority.h network.h globals.h receiver.h
	$(CC) $(CFLAGS) -c sender.c

receiver.o: receiver.c receiver.h event.h heap_priority.h network.h globals.h sender.h
	$(CC) $(CFLAGS) -c receiver.c

network.o: network.c network.h event.h
	$(CC) $(CFLAGS) -c network.c

clean:
	rm -f $(OBJS) sim

.PHONY: all clean

CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11

OBJS = main.o heap_priority.o sender.o receiver.o network.o

all: sim

sim: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c event.h heap_priority.h sender.h receiver.h network.h
heap_priority.o: heap_priority.c heap_priority.h event.h
sender.o: sender.c sender.h event.h heap_priority.h network.h
receiver.o: receiver.c receiver.h event.h heap_priority.h network.h
network.o: network.c network.h event.h heap_priority.h

clean:
	rm -f $(OBJS) sim

.PHONY: all clean

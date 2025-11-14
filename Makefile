# CC:
#   This variable holds the name of the C compiler we want to use.
#   Here it is set to "gcc", the standard GNU C compiler.
CC = gcc

# CFLAGS:
#   These are the compilation flags passed to gcc when compiling .c into .o files.
#   -O2       → optimization level 2 (balanced speed/size)
#   -Wall     → enable all common warning messages
#   -Wextra   → enable extra warning messages (more strict)
#   -std=c11  → tell gcc to use the C11 standard
CFLAGS = -O2 -Wall -Wextra -std=c11

# OBJS:
#   This is a list of ALL object (.o) files required to build the final program.
#   Each .o file corresponds to a .c file in the project.
#   By grouping them into one variable, we avoid repeating the list multiple times.
OBJS = main.o heap_priority.o sender.o receiver.o network.o

###############################################################################
# RULE: "all"
#   This is the default rule when you run simply:   make
#   The target "all" depends on "sim".
#   So running "make" means "build the 'sim' executable".
###############################################################################
all: simulation_out


###############################################################################
# RULE: "simulation_out"
#   This rule links all object files together into the final executable named "simulation_out".
#
#   sim: $(OBJS)
#       $(CC) $(CFLAGS) -o $@ $(OBJS)
#
#   - sim depends on all .o files listed in $(OBJS).
#   - If any .o file is missing or outdated, Make will rebuild it automatically.
#   - $(CC) = gcc
#   - $(CFLAGS) are the compiler flags defined above
#   - -o $@   → output file is the target's name (“sim”)
#   - $(OBJS) → all object files
###############################################################################
simulation_out: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)


###############################################################################
# COMPILATION RULES FOR EACH .o FILE
#
# Make automatically knows how to build:
#   *.o from *.c using the $(CC) and $(CFLAGS)
#
# But it does NOT know which header files affect each .o file.
# We list dependencies so Make can recompile .o files only if needed.
#
# Example:
#   main.o depends on main.c AND event.h AND heap_priority.h etc.
#   If any of these headers change, Make will recompile only main.o.
###############################################################################

# main.o depends on these header files:
main.o: main.c event.h heap_priority.h sender.h receiver.h network.h

# heap_priority.o depends on its source file AND its headers:
heap_priority.o: heap_priority.c heap_priority.h event.h

# sender.o depends on sender.h and other relevant modules:
sender.o: sender.c sender.h event.h heap_priority.h network.h

# receiver.o similarly depends on these:
receiver.o: receiver.c receiver.h event.h heap_priority.h network.h

# network.o depends on the networking logic and event/heap headers:
network.o: network.c network.h event.h heap_priority.h


###############################################################################
# RULE: "clean"
#
# This rule deletes:
#   - all object files (*.o)
#   - the final executable (sim)
#
# This is useful for:
#   - cleaning build artifacts
#   - forcing a full rebuild next time
#
# Running:
#   make clean
#
# Always succeeds because we list clean under .PHONY (see below).
###############################################################################
clean:
	rm -f $(OBJS) simulation_out


###############################################################################
# .PHONY:
#   Declares targets that are NOT actual files.
#   Without this, Make might get confused if files named "all" or "clean" exist.
#   Declaring them as .PHONY ensures they always run when called.
###############################################################################
.PHONY: all clean

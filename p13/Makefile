CC     = gcc
CFLAGS = -Wall -g 
CLIBS = -lrt

PROG = pingpong-disco1 pingpong-disco2
OBJS = disk.o ppos_disk.o ppos_core.o queue.o

.PHONY: clean purge all

all: $(PROG)

%.o: %.c
	$(CC) -c $(CFLAGS) $<

$(PROG) : % :  $(OBJS) %.o
	$(CC) -o $@ $^ $(CLIBS)

clean:
	@rm -f *.o

purge: clean
	@rm -f $(PROG)

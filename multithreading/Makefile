CC = gcc
CFLAGS = -ansi -pedantic -Wall -g
LDFLAGS = -lpthread
SRCMODULES =
OBJMODULES = $(SRCMODULES:.c=.o)

all: server.out

server.out: simple_tcp.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -rf *.o *.gch *.out

CC = gcc
CFLAGS = -ansi -pedantic -Wall -g
SRCMODULES = client_sess.c client_list.c
OBJMODULES = $(SRCMODULES:.c=.o)

all: server.out

server.out: simple_tcp.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -rf *.o *.gch *.out

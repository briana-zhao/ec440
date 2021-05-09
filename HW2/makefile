CC=gcc
CFLAGS=-std=gnu99 -I -g -Wall -U_FORTIFY_SOURCE -c -O0
DEPS= ec440threads.h

threads: threads.o
	$(CC) $(CFLAGS) -o threads threads.o

clean:
	rm threads


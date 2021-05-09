CC=gcc
CFLAGS=-std=gnu99 -I -g -Wall -Werror -pedantic -O0 -lpthread -c
DEPS= tls.h

tls: tls.o
	$(CC) $(CFLAGS) -o tls tls.o

clean:
	rm tls


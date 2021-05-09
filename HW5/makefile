CC=gcc
CFLAGS=-std=gnu99 -I -g -Wall -Werror -pedantic -O0 -c
DEPS= fs.h disk.h

fs: fs.o disk.o
	$(CC) $(CFLAGS) -o fs fs.o disk.o

clean:
	rm fs


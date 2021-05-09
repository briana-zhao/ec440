CC=gcc
CFLAGS=-std=gnu99 -I -g -Wall

myshell: myshell.o myshell_parser.o
	$(CC) $(CFLAGS) -o myshell myshell.o myshell_parser.o

clean:
	rm myshell


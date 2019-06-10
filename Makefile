CC=gcc
CPPFLAGS= -I. \
		  -D_POSIX_C_SOURCE=200809L
LDLIBS=-lpthread
CFLAGS=	-std=c99 \
		-Wall

linkedlist.o: linkedlist.c linkedlist.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

test: test.c test.h linkedlist.o
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) $< linkedlist.o -o $@

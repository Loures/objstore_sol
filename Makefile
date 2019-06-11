DEBUG=-DDEBUG
CC=gcc
CPPFLAGS= -I. \
		  -D_POSIX_C_SOURCE=200809L \
		  $(DEBUG)
LDLIBS=-lpthread
CFLAGS=	-std=c99 \
		-Wall

default_target: server

linkedlist.o: linkedlist.c linkedlist.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

test: test.c test.h linkedlist.o
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) $< linkedlist.o -o $@

dispatcher.o: dispatcher.c dispatcher.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

worker.o: worker.c worker.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

server.o: server.c server.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

server: server.o worker.o dispatcher.o linkedlist.o
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) linkedlist.o dispatcher.o worker.o $< -o $@

clean:
	rm ./*.o ./server

.PHONY: clean

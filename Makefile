DEBUG=-DDEBUG
CC=gcc
CPPFLAGS= -I. \
		  -D_POSIX_C_SOURCE=200809L \
		  $(DEBUG)
LDLIBS=-lpthread
CFLAGS=	-std=c99 \
		-Wall
		
linkedlist.o: linkedlist.c linkedlist.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

test: test.c test.h linkedlist.o
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) $< linkedlist.o -o $@

dispatcher.o: dispatcher.c dispatcher.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

worker.o: worker.c worker.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

main.o: main.c main.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

main: main.o worker.o dispatcher.o linkedlist.o
		$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) linkedlist.o dispatcher.o worker.o $< -o $@

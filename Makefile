DEBUG=-DDEBUG
CC=gcc
CPPFLAGS= -I. \
		  -D_POSIX_C_SOURCE=200809L \
		  $(DEBUG)
LDLIBS=-lpthread
CFLAGS=	-std=c99 \
		-Wall \
		-g


default_target: os_server

os_client.o: os_client.c os_client.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

linkedlist.o: linkedlist.c linkedlist.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

fs.o: fs.c fs.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

test: test.c test.h
	$(CC) $(CFLAGS) $(LDLIBS) -lreadline $(CPPFLAGS) $< linkedlist.o -o $@

interactive: interactive.c
	$(CC) $(CFLAGS) $(LDLIBS) -lreadline $(CPPFLAGS) $< -o $@

dispatcher.o: dispatcher.c dispatcher.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

worker.o: worker.c worker.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

os_server.o: os_server.c os_server.h
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

os_server: os_server.o worker.o dispatcher.o linkedlist.o os_client.o fs.o
	$(CC) $(CFLAGS) $(LDLIBS) $(CPPFLAGS) linkedlist.o dispatcher.o worker.o os_client.o fs.o $< -o $@

clean:
	rm ./*.o ./os_server

.PHONY: clean

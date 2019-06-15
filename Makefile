DEBUG=-DDEBUG
CC=gcc
CPPFLAGS= -I. \
		  -D_POSIX_C_SOURCE=200809L \
		  $(DEBUG)
LDLIBS=-lpthread
LDFLAGS=-L.
CFLAGS=	-std=c99 \
		-Wall \
		-g

SRV_OBJECTS=os_server.o worker.o dispatcher.o linkedlist.o os_client.o fs.o

default_target: all

libobjstore.a: objstore.o objstore.h
	ar rsv $@ $<
	ranlib $@

os_client.o: os_client.c os_client.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

linkedlist.o: linkedlist.c linkedlist.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

fs.o: fs.c fs.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

test: test.c libobjstore.a
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(CPPFLAGS) $< -o $@ -lobjstore

interactive: interactive.c libobjstore.a
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -lreadline $(CPPFLAGS) $< -o $@ -lobjstore

objstore: objstore.c objstore.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

dispatcher.o: dispatcher.c dispatcher.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

worker.o: worker.c worker.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

os_server.o: os_server.c os_server.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(CPPFLAGS) -c $< -o $@

os_server: $(SRV_OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(CPPFLAGS) $(SRV_OBJECTS) -o $@

all: os_server test interactive

.PHONY: clean

clean:
	rm ./*.o ./os_server


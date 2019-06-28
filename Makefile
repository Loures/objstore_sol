CPPFLAGS=-I. -D_POSIX_C_SOURCE=200809L
LDFLAGS=-L.
LIBNAME=objstore
CFLAGS=-std=c99 -Wall
SHELL=/usr/bin/env bash
		
SRV_OBJECTS=os_server.o worker.o dispatcher.o linkedlist.o os_client.o fs.o

default_target: all

linkedlist.o: linkedlist.c linkedlist.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS) -c $< -o $@ -lpthread

dispatcher.o: dispatcher.c dispatcher.h linkedlist.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS) -c $< -o $@ -lpthread

worker.o: worker.c worker.h linkedlist.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS) -c $< -o $@ -lpthread

fs.o: fs.c fs.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS) -c $< -o $@

os_client.o: os_client.c os_client.h linkedlist.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS) -c $< -o $@

os_server: $(SRV_OBJECTS)
	$(CC) $(CFLAGS) -O3 $(LDFLAGS) $(CPPFLAGS) $(SRV_OBJECTS) -o $@ -lpthread

lib$(LIBNAME).a: $(LIBNAME).o $(LIBNAME).h
	$(AR) rcs $@ $<

client: client.c libobjstore.a
	$(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS) -DERRSTR=objstore_errstr $< -o $@ -l$(LIBNAME)

interactive: interactive.c interactive.h libobjstore.a
	$(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS) -DERRSTR=objstore_errstr $< -o $@ -l$(LIBNAME) -lreadline

bigblock: bigblock.c libobjstore.a
	$(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS) -DERRSTR=objstore_errstr $< -o $@ -l$(LIBNAME)

all: os_server libobjstore.a client 

testing: CFLAGS+=-g
testing: all

test: cleardata
	@./os_server &
	@./testscript.sh 50

clean:
	$(RM) ./*.o ./os_server client libobjstore.a interactive bigblock

cleardata:
	@$(RM) -r data/*


.PHONY: clean test cleardata

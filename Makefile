SOCKET_ADDR=./objstore.sock
CPPFLAGS=-I. -D_POSIX_C_SOURCE=200809L -DSOCKET_ADDR=\"$(SOCKET_ADDR)\"
LIBNAME=objstore
CFLAGS=-std=c99 -Wall
SHELL=/usr/bin/env bash
		
SRV_OBJECTS=worker.o dispatcher.o myhash.o os_client.o fs.o

default_target: all

%.o: %.c %.h os_server.h

lib$(LIBNAME).a: $(LIBNAME).o $(LIBNAME).h
	$(AR) rcs $@ $<

os_server: $(SRV_OBJECTS) os_server.c os_server.h
	$(CC) $(CFLAGS) -O3 $(CPPFLAGS) $(SRV_OBJECTS) os_server.c -o $@ -lpthread

client: client.c libobjstore.a
	$(CC) $(CFLAGS) -L. $(CPPFLAGS) -DERRSTR=objstore_errstr $< -o $@ -l$(LIBNAME)

interactive: interactive.c libobjstore.a
	$(CC) $(CFLAGS) -L. $(CPPFLAGS) -DERRSTR=objstore_errstr $< -o $@ -l$(LIBNAME) -lreadline

bigblock: bigblock.c libobjstore.a
	$(CC) $(CFLAGS) -L. $(CPPFLAGS) -DERRSTR=objstore_errstr $< -o $@ -l$(LIBNAME)

all: os_server libobjstore.a client 
	@$(RM) $(SOCKET_ADDR)

testing: CFLAGS+=-g
testing: all

test: cleardata
	@./os_server &
	@./testscript.sh 50

clean: cleardata
	$(RM) $(SOCKET_ADDR) ./*.o ./os_server client libobjstore.a interactive bigblock

cleardata:
	@$(RM) -r data/*


.PHONY: clean test cleardata

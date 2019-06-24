CC=gcc
CPPFLAGS=-I. -D_POSIX_C_SOURCE=200809L
LDFLAGS=-L.
LIBNAME=objstore
CFLAGS=-std=c99 -Wall 
		
SRV_OBJECTS=os_server.o worker.o dispatcher.o linkedlist.o os_client.o fs.o

default_target: all

%.o: %.c %.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS) -c $< -o $@ -lpthread

lib$(LIBNAME).a: $(LIBNAME).o $(LIBNAME).h
	$(AR) rcsv $@ $<

fs.o: fs.c fs.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS) -c $< -o $@

os_client.o: os_client.c os_client.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS) -c $< -o $@

client: client.c libobjstore.a
	$(CC) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS) $< -o $@ -l$(LIBNAME)

os_server: $(SRV_OBJECTS)
	$(CC) $(CFLAGS) -O3 $(LDFLAGS) $(CPPFLAGS) $(SRV_OBJECTS) -o $@ -lpthread

all: CFLAGS+=-g
all: os_server libobjstore.a client #sakuraba

release: os_server libobjstore.a client

dotest:
	@$(RM) -r data/*
	@./testscript.sh

clean:
	$(RM) ./*.o ./os_server client libobjstore.a

#test:	//fill this space

.PHONY: clean

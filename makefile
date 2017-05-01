CC=gcc
CFLAGS= -g -Wall
OBJS=libnetfiles.o utils.o

all: libnetfiles.o netfileserver

libnetfiles.o: libnetfiles.c libnetfiles.h utils.o
	$(CC) $(CFLAGS) -c libnetfiles.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

netfileserver: netfileserver.c netfileserver.h utils.o
	$(CC) $(CFLAGS) -pthread -o $@ netfileserver.c utils.o

testclient: testclient.c utils.o
	$(CC) -o $@ testclient.c utils.o

testdriver: driver.c libnetfiles.o
	$(CC) -o $@ driver.c $(OBJS)

clean:
	rm -f $(OBJS) netfileserver testclient testdriver

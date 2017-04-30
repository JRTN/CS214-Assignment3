CC=gcc
CFLAGS= -g -Wall
OBJS=libnetfiles.o

libnetfiles.o: libnetfiles.c libnetfiles.h
	$(CC) $(CFLAGS) -c libnetfiles.c

netfileserver: netfileserver.c
	$(CC) $(CFLAGS) -pthread -o $@ netfileserver.c

testclient: testclient.c
	$(CC) -o $@ testclient.c

testdriver: driver.c libnetfiles.o
	$(CC) -o $@ driver.c libnetfiles.o

clean:
	rm -f $(OBJS) netfileserver testclient testdriver

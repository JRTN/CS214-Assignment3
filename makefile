CC=gcc
CFLAGS= -g -Wall
OBJS=libnetfiles.o

libnetfiles.o: libnetfiles.c libnetfiles.h
	$(CC) $(CFLAGS) -c libnetfiles.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

netfileserver: netfileserver.c netfileserver.h utils.o
	$(CC) $(CFLAGS) -pthread -o $@ netfileserver.c utils.o

testclient: testclient.c
	$(CC) -o $@ testclient.c

testdriver: driver.c libnetfiles.o
	$(CC) -o $@ driver.c libnetfiles.o

clean:
	rm -f $(OBJS) netfileserver testclient testdriver

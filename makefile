CC=gcc
CFLAGS= -g -Wall -std=c99
OBJS=libnetfiles.o

libnetfiles.o: libnetfiles.c libnetfiles.h
	$(CC) $(CFLAGS) -c libnetfiles.c

netfileserver: netfileserver.c
	$(CC) $(CFLAGS) -pthread -o $@ netfileserver.c

testclient: testclient.c
	$(CC) -o $@ testclient.c

testserver: testserver.c
	$(CC) -o $@ testserver.c

clean:
	rm -f $(OBJS) netfileserver testclient testserver

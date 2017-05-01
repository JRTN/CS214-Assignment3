#include "libnetfiles.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define TESTF1 "test.txt"

int main(int argc, char **argv) {
    if(argc < 2) {
        fprintf(stderr, "Need host name\n");
        return 0;
    }
    int check = netserverinit(argv[1]);
    printf("returns %d from trying to connect to %s\n",check, argv[1]);
    if(check == -1){
        return -1;
    }
    int filed = netopen(TESTF1, O_RDWR);
    printf("file descriptor: %d\n", filed);
    if(filed == -1) {
        printf("Errno: %d\n", errno);
    }
    ssize_t writeres = netwrite(filed, "abc", 3);
    printf("write res: %zd\n", writeres);
    if(writeres == -1) {
        printf("Errno: %d\n", errno);
    }
    int dummyfd = netopen("doesntexist", O_RDONLY);
    printf("file descriptor: %d\n", dummyfd);
    if(dummyfd == -1) {
        printf("Errno:  %d\n", errno);
    }
    char buffer[6] = {0};
    ssize_t readres = netread(filed, buffer, 5);
    printf("readbytes: %zd\n", readres);
    printf("buffer: %s\n", buffer);
    int closed = netclose(filed);
    printf("close: %d\n", closed);
    close(filed);
    return 0;
}

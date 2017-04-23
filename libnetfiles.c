#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "libnetfiles.h"

static int socketdesc = -2;

static int createSocket() {
    int newsocket = socket(AF_INET, SOCK_STREAM, 0);
    if(newsocket == -1) {
        fprintf(stderr, "[%s: %d] Failed to create socket", __FILE__, __LINE__);
    }
    return newsocket;
}

int netopen(const char *pathname, int flags) {
    /*
        Open socket connection to file server
    */
    return 0;
}

ssize_t netread(int fildes, void *buf, size_t nbyte) {
    return 0;
}

ssize_t netwrite(int fildes, const void *buf, size_t nbyte) {
    return 0;
}

int netclose(int fd) {
    return 0;
}

int netserverinit(char *hostname) {
    /*
        Set IP address of remote machine for library
    */
    return 0;
}

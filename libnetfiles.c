#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "libnetfiles.h"

static int sockfd = -1;

void errormsg(const char const * msg, const char const *file, const int line) {
    fprintf(stderr, "[%s : %d] %s\n", file, line, msg);
}

static int sendMessageToSocket(const char const *message) {
    int wrotebits = 0;
    if((wrotebits = write(sockfd, message, strlen(message))) < 0) {
        errormsg("ERROR writing to socket", __FILE__, __LINE__);
        return -1;
    }
    return wrotebits;
}

static void printSocketResponse() {
    char buffer[256] = {0};
    int readbits = 0;
    if((readbits = read(sockfd, buffer, 255)) < 0) {
        errormsg("Error reading socket response", __FILE__, __LINE__);
    }
    printf("%s\n", buffer);
}

static char *buildOpenRequest(const char *pathname, int flags) {
    size_t pathlen = strlen(pathname);
    //extra spaces needed: 1 for null terminator, 1 for flag, 1 for mode character
    //request is of the form [r|w|o][1|2|3][pathname]
    char *omessage = malloc(pathlen + 3);
    sprintf(omessage, "%c%d%s", 'o', flags, pathname);
    return omessage;
}

int netopen(const char *pathname, int flags) {
    //create request containing necessary information
    char *orequest = buildOpenRequest(pathname, flags);
    //send request to server
    int wrotebits = sendMessageToSocket(orequest);
    free(orequest);
    if(wrotebits < 0) {
        //error
    }
    //read server's response
    printSocketResponse();
    return 0;
}

static char *buildReadRequest(int fildes, size_t nbyte) {
    char *rrequest = malloc(200);
    sprintf(rrequest, "%s", "Read Request");
    return rrequest;
}

ssize_t netread(int fildes, void *buf, size_t nbyte) {
    char *rrequest = buildReadRequest(fildes, nbyte);
    int wrotebits = sendMessageToSocket(rrequest);
    free(rrequest);
    if(wrotebits < 0) {
        //error
    }
    //read server's response
    printSocketResponse();
    return 0;
}

static char *buildWriteRequest(int fildes, const void *buf, size_t nbyte) {
    return 0;
}

ssize_t netwrite(int fildes, const void *buf, size_t nbyte) {
    return 0;
}

int netclose(int fd) {
    return 0;
}

int netserverinit(char *hostname) {
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        errormsg("ERROR opening socket", __FILE__, __LINE__);
        return -1;
    }
    struct hostent *server;
    if(!(server = gethostbyname(hostname))) {
        errormsg("ERROR no such host", __FILE__, __LINE__);
        return -1;
    }
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    memcpy((void *)&serv_addr.sin_addr.s_addr, (void *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(PORT);
    if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        errormsg("ERROR can't connect to host", __FILE__, __LINE__);
        return -1;
    }
    return 0;
}

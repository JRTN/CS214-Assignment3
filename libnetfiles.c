#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "libnetfiles.h"

#define MAXINT 2147483647
#define MININT -2147483648

static int sockfd = -1;

void errormsg(const char const * msg, const char const *file, const int line) {
    fprintf(stderr, "[%s : %d] %s\n", file, line, msg);
}

int numPlaces (int n) {
    if (n < 0) n = (n == MININT) ? MAXINT : -n;
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    /*      2147483647 is 2^31-1 - add more ifs as needed
       and adjust this final return as well. */
    return 10;
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
    int fildes_size = numPlaces(fildes);
    int nbyte_size = numPlaces(fildes);
    
    //SEGMENTATION FAULT
    char *wrequest = malloc(fildes_size + nbyte_size + nbyte + 5);
    sprintf(wrequest, "%c#%d#%zu#", 'w', fildes, nbyte);
    strncpy((char *)(buf + fildes_size + nbyte_size), (char*)buf, nbyte);
    wrequest[fildes_size + nbyte_size + nbyte + 4] = 0;
    return wrequest;
}

ssize_t netwrite(int fildes, const void *buf, size_t nbyte) {
    char *wrequest = buildWriteRequest(fildes, buf, nbyte);
    int wrotebits = sendMessageToSocket(wrequest);
    free(wrequest);
    if(wrotebits < 0) {
        //error
    }
    printSocketResponse();
    return 0;
}

int netclose(int fd) {
    return 0;
}

int netserverinit(char *hostname) {
    //Create socket with socket() system call
    int sockfd = 0;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        errormsg("ERROR opening socket", __FILE__, __LINE__);
        return -1;
    }
    //Get server information
    struct hostent *server;
    if(!(server = gethostbyname(hostname))) {
        errormsg("ERROR no such host", __FILE__, __LINE__);
        return -1;
    }
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    memcpy((void *)&serv_addr.sin_addr.s_addr, (void *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(PORT);
    //Connect the socket to the address of the server using the connect() system call
    if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        errormsg("ERROR can't connect to host", __FILE__, __LINE__);
        return -1;
    }
    return sockfd;
}

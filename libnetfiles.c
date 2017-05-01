#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <errno.h>

#include "libnetfiles.h"
#include "utils.h"

#define MAXINT 2147483647
#define MAX_INT_LENGTH 11
#define MININT -2147483648
#define CHUNK_SIZE 5

#define DEV false

static int sockfd = -1;

static char *buildOpenRequest(const char *pathname, int flags) {
    size_t pathlen = strlen(pathname);
    int messageSize = pathlen + 5; //1 for flag, 1 for o, 2 for delims, 1 for null
    char *omessage = malloc(messageSize);
    sprintf(omessage, "o!%d!%s", flags, pathname);
    return omessage;
}

static int parseOpenResponse(char *resp){
    int fd = strtol(resp, NULL, 10);
    if(fd == -1) {
        char *err = strchr(resp, DELIMITER) + 1;
        seterrno(err);
    }
    return fd;
}

int netopen(const char *pathname, int flags) {
    if(flags != O_RDONLY && flags != O_WRONLY && flags != O_RDWR) {
        errno = EINVFLAG;
        return -1; //invalid flags
    }
    char *orequest = buildOpenRequest(pathname, flags);
    if(DEV) printf("message: %s\n",orequest);
    packet *reqpkt = packetCreate(orequest, strlen(orequest) + 1);
    int sendres = sendPacket(sockfd, reqpkt);
    packetDestroy(reqpkt);
    if(sendres < 1) {
        return -1; //error sending packet
    }
    packet *respkt = readPacket(sockfd);
    if(!respkt) {
        return -1; //error reading packet
    }
    int result = parseOpenResponse(respkt->data);
    packetDestroy(respkt);
    return result;
}

static char *buildReadRequest(int fildes, size_t nbyte) {
    int messageSize = count_digits(nbyte) + count_digits(fildes) + 4;
    char *rrequest = malloc(messageSize);
    sprintf(rrequest, "r!%zu!%d" , nbyte,fildes);
    return rrequest;
}
ssize_t parseReadResponse(char* resp, void *buf){
    ssize_t readbytes = strtol(resp, NULL, 10);
    if(readbytes > 0) {
        void *data = strchr(resp, DELIMITER) + 1;
        memcpy(buf, data, readbytes);
    }
    return readbytes;
}

ssize_t netread(int fildes, void *buf, size_t nbyte) {
    if(DEV) printf("file descriptor %d\n", fildes);
    char *rrequest = buildReadRequest(fildes, nbyte);
    if(DEV) printf("message: %s\n",rrequest );

    packet *reqpkt = packetCreate(rrequest, strlen(rrequest) + 1);
    int sendres = sendPacket(sockfd, reqpkt);
    packetDestroy(reqpkt);
    if(sendres < 1) {
        return -1;
    }
    packet *respkt = readPacket(sockfd);
    if(!respkt) {
        return -1;
    }
    ssize_t result = parseReadResponse(respkt->data, buf);
    packetDestroy(respkt);
    return result;
}

static char *buildWriteRequest(int fildes, const void *buf, size_t nbyte) {
    if(DEV) printf("file descriptor %d\n", fildes);
    int messagesize = count_digits(fildes) + count_digits(nbyte) + nbyte + 5; //1 for w, 3 for delimiters, 1 for null terminator
    char *wrequest = malloc(messagesize);
    snprintf(wrequest, messagesize, "w!%d!%zu!%s", fildes, nbyte, (char *)buf);
    return wrequest;
}

ssize_t parseWriteResponse(char *resp){
    ssize_t wrotebytes = strtol(resp, NULL, 10);
    if(wrotebytes < 0) {
        char *err = strchr(resp, DELIMITER) + 1;
        seterrno(err);
    }
    return wrotebytes;
}

ssize_t netwrite(int fildes, const void *buf, size_t nbyte) {
    char *request = buildWriteRequest(fildes, buf, nbyte);
    packet *reqpkt = packetCreate(request, strlen(request) + 1);
    int sendres = sendPacket(sockfd, reqpkt);
    packetDestroy(reqpkt);
    if(sendres < 1) {
        return -1;
    }
    packet *respkt = readPacket(sockfd);
    if(!respkt) {
        return -1;
    }
    ssize_t result = parseWriteResponse(respkt->data);
    packetDestroy(respkt);
    return result;
}

static char *buildCloseRequest(int fildes){
    if(DEV) printf("file descriptor %d\n", fildes);
    int messagesize = count_digits(fildes) + 3; //1 for c, 1 for delimiter, 1 for terminator
    char *crequest= malloc(messagesize);
    sprintf(crequest, "c!%d", fildes);
    return crequest;
}

int parseCloseResponse(char *resp){
    int fd = strtol(resp, NULL, 10);
    if(fd == -1) {
        char *err = strchr(resp, DELIMITER) + 1;
        seterrno(err);
    }
    return fd;
}

int netclose(int fd) {
    char *request = buildCloseRequest(fd);
    packet *reqpkt = packetCreate(request, strlen(request) + 1);
    int sendres = sendPacket(sockfd, reqpkt);
    packetDestroy(reqpkt);
    if(sendres < 1) {
        return -1;
    }
    packet *respkt = readPacket(sockfd);
    if(!respkt) {
        return -1;
    }
    int result = parseCloseResponse(respkt->data);
    packetDestroy(respkt);
    return result;
}

/*
    Initializes a connection to the server indicated by the hostname.
*/
int netserverinit(char *hostname) {
    //Create socket with socket() system call
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        if(DEV) errormsg("ERROR opening socket", __FILE__, __LINE__);
        return -1;
    }
    //Get server information
    struct hostent *server;
    if(!(server = gethostbyname(hostname))) {
        if(DEV) errormsg("ERROR no such host", __FILE__, __LINE__);
        return -1;
    }
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    memcpy((void *)&serv_addr.sin_addr.s_addr, (void *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(PORT);
    //Connect the socket to the address of the server using the connect() system call
    if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        if(DEV) errormsg("ERROR can't connect to host", __FILE__, __LINE__);
        return -1;
    }
    if(DEV) printf("socket %d\n", sockfd);
    return 0;
}

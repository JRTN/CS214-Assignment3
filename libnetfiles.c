#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>

#include "libnetfiles.h"

#define MAXINT 2147483647
#define MAX_INT_LENGTH 11
#define MININT -2147483648
#define CHUNK_SIZE 5

static int sockfd = -1;

static char *buildOpenRequest(const char *pathname, int flags) {
    size_t pathlen = strlen(pathname);
    int messageSize = pathlen + 5; //1 for flag, 1 for o, 2 for delims, 1 for null
    char *omessage = malloc(messageSize);
    sprintf(omessage, "o!%d!%s", flags, pathname);
    return omessage;
}

static int handleOpenResponse(){
    packet *responsepkt = readPacket(sockfd);
    
    packetDestroy(responsepkt);
    return fd;
}

int netopen(const char *pathname, int flags) {
    char *orequest = buildOpenRequest(pathname, flags);
    printf("message: %s\n",orequest);
    //send request to server
    int wrotebits = sendMessageToSocket(orequest);
    free(orequest);
    if(wrotebits < 0) {
      //error
    }
    //read server's response
    parseOpenResponse();

    return 0;
}



static char *buildReadRequest(int fildes, size_t nbyte) {
    char *rrequest = malloc(200);
    int messageSize = strlen(intToStr(fildes))+strlen(intToStr(nbyte))+5;
    sprintf(rrequest, "%d!r!%zu!%d",messageSize,nbyte,fildes);
    return rrequest;
}
ssize_t parseReadResponse(char* buf){
  //char buffer[256] = {0};
  int readbits = 0;
  if((readbits = read(sockfd, buf, 255)) < 0) {
      errormsg("Error reading socket response", __FILE__, __LINE__);
  }
  printf("response: %s\n",buf);
  int size = getMessageSize(buf);
  //int flag = *(buf + strlen(size) + 1) - '0';
  buf = safeAdvanceCharacters(buf, strlen(intToStr(size))+1);
  if(!*buf) {
      //error, hit end of string before finding pathname
  }
  int nbyte = buildToken(buf, DELIMITER, true);
  buf = safeAdvanceCharacters(buf, strlen(intToStr(size))+1);
  if(nbyte == -1){
    //error
      return -1;
  }
  printf("%s\n",buf);
  return nbyte;
}

ssize_t netread(int fildes, void *buf, size_t nbyte) {
    printf("file descriptor %d\n", fildes);
    char *rrequest = buildReadRequest(fildes, nbyte);
    printf("message: %s\n",rrequest );
    int wrotebits = sendMessageToSocket(rrequest);
    free(rrequest);
    if(wrotebits < 0) {
        //error
    }
    //read server's response

    return parseReadResponse((char*)buf);
}



static char *buildWriteRequest(int fildes, const void *buf, size_t nbyte) {
    printf("file descriptor %d\n", fildes);
    char nullTermBuf[nbyte+1];
    nullTermBuf[nbyte] = '\0';
    memcpy(nullTermBuf, buf, nbyte);
    char *wrequest = malloc(strlen(intToStr(fildes)) + strlen(intToStr(nbyte))+nbyte + 12);
    int messageSize = strlen(intToStr(fildes)) + strlen(intToStr(nbyte))+nbyte+6;
    sprintf(wrequest, "%d!w!%d!%zu!%s",messageSize,fildes,nbyte,nullTermBuf);
    return wrequest;
}

ssize_t parseWriteResponse(){
  char* buffer = {0};
  int readbits = 0;
  if((readbits = read(sockfd, buffer, 255)) < 0) {
      errormsg("Error reading socket response", __FILE__, __LINE__);
  }
  printf("response: %s\n",buffer);
  int size = getMessageSize(buffer);
  buffer = safeAdvanceCharacters(buffer, strlen(intToStr(size))+1);
  if(!*buffer) {
      //error, hit end of string before finding pathname
  }
  int nbyte = atoi(buildToken(buffer, DELIMITER, true));
  printf("%s\n",intToStr(nbyte));
  return nbyte;
}

ssize_t netwrite(int fildes, const void *buf, size_t nbyte) {
    printf("file descriptor %d\n", fildes);
    char *wrequest = buildWriteRequest(fildes, buf, nbyte);
    printf("message: %s\n",wrequest);
    int wrotebits = sendMessageToSocket(wrequest);
    free(wrequest);
    if(wrotebits < 0) {
        //error
    }
    int readBytes = parseWriteResponse();
    if(readBytes>nbyte){
      //error
    }
    return 0;
}



static char *buildCloseRequest(int fildes){
    printf("file descriptor %d\n", fildes);
    int messageSize = 3+strlen(intToStr(fildes));
    char *cmessage = malloc(messageSize);
    sprintf(cmessage, "%d!c!%d",messageSize, fildes);
    return cmessage;
}


int netclose(int fd) {
    char *crequest = buildCloseRequest(fd);
    //send request to server
    printf("%s\n",crequest);
    int wrotebits = sendMessageToSocket(crequest);
    free(crequest);
    if(wrotebits < 0) {
        //error
    }
    //read server's response
    return parseCloseResponse();
}

int parseCloseResponse(){
    char* buffer = {0};
    int readbits = 0;
    if((readbits = read(sockfd, buffer, 255)) < 0) {
      errormsg("Error reading socket response", __FILE__, __LINE__);
      return -1;
    }
    printf("response: %s\n",buffer);
    int size = getMessageSize(buffer);
    buffer = safeAdvanceCharacters(buffer, strlen(intToStr(size))+1);
    if(!*buffer) {
        //error, hit end of string before finding pathname
    }
    int num = atoi(buildToken(buffer, DELIMITER, true));
    if(num == -1){
      buffer = safeAdvanceCharacters(buffer, strlen(intToStr(size))+1);
      char* error = buildToken(buffer,DELIMITER,true);
      return -1;
    }
    return 0;

}

int netserverinit(char *hostname) {
    //Create socket with socket() system call
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
    printf("socket %d\n", sockfd);
    return 0;
}

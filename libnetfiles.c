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
/*
char * errnoToCode() {
    switch(errno) {
        case EACCES:          return strdup("EACCES");
        case EINTR:           return strdup("EINTR");
        case EISDIR:          return strdup("EISDIR");
        case ENOENT:          return strdup("ENOENT");
        case ENFILE:          return strdup("ENFILE");
        case EWOULDBLOCK:     return strdup("EWOULDBLOCK");
        case ETIMEDOUT:       return strdup("ETIMEDOUT");
        case EBADF:           return strdup("EBADF");
        case ECONNRESET:      return strdup("ECONNRESET");
        case HOST_NOT_FOUND:  return strdup("HOST_NOT_FOUND");
        default: return NULL;
    }
}
*/

char * buildToken(const char *str, const char delim, bool usedelim) {
    if(!str) return NULL;
    size_t size = 0;
    size_t capacity = CHUNK_SIZE;
    char *token = malloc(capacity);
    /*  If usedlim is set, then we must check that the current character is not the
        delimiter. If it is not set, then we don't check and continue on (which is
        the function of the (!usedelim) check) */
    while(*str && ((usedelim && *str != delim) || (!usedelim)) ) {
        size++;
        if(size >= capacity) {
            capacity += CHUNK_SIZE;
            token = realloc(token, capacity);
        }
        token[size - 1] = *str++;
    }
    if(size <= 0) {
        free(token);
        return NULL;
    }
    token = realloc(token, size + 1);
    token[size] = '\0';
    return token;
}
void errormsg(const char const * msg, const char const *file, const int line) {
    fprintf(stderr, "[%s : %d] %s\n", file, line, msg);
}

long int getMessageSize(const char const *message) {
    char *sizestr = buildToken(message, DELIMITER, true);
    long int result = strtol(sizestr, NULL, 10);
    free(sizestr);
    return result;
}


char * intToStr(const int num) {
    char *result = malloc(MAX_INT_LENGTH);
    int ret = snprintf(result, MAX_INT_LENGTH, "%d", num);
    result = realloc(result, ret + 1);
    return result;
}


char * safeAdvanceCharacters(const char *str, const size_t amt) {
    int i;
    for(i = 0; i < amt && *str; i++) str++;
    return (char *)str;
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
    printf("file descriptor %d\n", fildes);
    int wrotebits = 0;
    if((wrotebits = write(sockfd, message, strlen(message))) < 0) {
        errormsg("ERROR writing to socket", __FILE__, __LINE__);
        return -1;
    }
    return wrotebits;
}

static void printSocketResponse() {
    printf("file descriptor %d\n", fildes);
    char buffer[256] = {0};
    int readbits = 0;
    if((readbits = read(sockfd, buffer, 255)) < 0) {
        errormsg("Error reading socket response", __FILE__, __LINE__);
    }
    printf("%s\n", buffer);
}

static char *buildOpenRequest(const char *pathname, int flags) {
    size_t pathlen = strlen(pathname);
    int messageSize = pathlen + 10;
    char *omessage = malloc(messageSize);
    sprintf(omessage, "%d!o!%d!%s",messageSize, flags, pathname);
    return omessage;
}
int parseOpenResponse(){
    char* buffer = {0};
    int readbits = 0;
    if((readbits = read(sockfd, buffer, 255)) < 0) {
        errormsg("Error reading socket response", __FILE__, __LINE__);
        return -1;
    }
    int size = getMessageSize(buffer);
    int flag = *(buffer + strlen(size) + 1) - '0';
    buffer = safeAdvanceCharacters(buffer, 2);
    if(!*buffer) {
        //error, hit end of string before finding pathname
    }
    int fd = buildToken(buffer, DELIMITER, true);
    if(fd == -1){
        buffer = safeAdvanceCharacters(buffer, 2);
        return 0;
    }
    printf("%s\n",intToStr(fd));
    return fd;
}

int netopen(const char *pathname, int flags) {
    printf("file descriptor %d\n", fildes);
    char *orequest = buildOpenRequest(pathname, flags);
    printf("%s\n",orequest);
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
    messageSize = messageSize+strlen(intToStr(messageSize));
    sprintf(rrequest, "%d!r!%zu!%d",messageSize,nbyte,fildes);
    return rrequest;
}
ssize_t parseReadResponse(char* buf){
  //char buffer[256] = {0};
  int readbits = 0;
  if((readbits = read(sockfd, buf, 255)) < 0) {
      errormsg("Error reading socket response", __FILE__, __LINE__);
  }
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
    int messageSize = strlen(intToStr(fildes)) + strlen(intToStr(nbyte))+nbyte;
    messageSize = messageSize + strlen(intToStr(messageSize));
    sprintf(wrequest, "%d!w!%d!%zu!%s",messageSize,fildes,nbyte,nullTermBuf);
    return wrequest;
}

ssize_t parseWriteResponse(){
  char* buffer = {0};
  int readbits = 0;
  if((readbits = read(sockfd, buffer, 255)) < 0) {
      errormsg("Error reading socket response", __FILE__, __LINE__);
  }
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
    int messageSize = 4+strlen(intToStr(fildes));
    messageSize = messageSize + strlen(intToStr(messageSize));
    char *cmessage = malloc(messageSize);
    sprintf(cmessage, "%d!c!%d",messageSize, fildes);
    return cmessage;
}


int netclose(int fd) {
    char *crequest = buildCloseRequest(fd);
    //send request to server
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

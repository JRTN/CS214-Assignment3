#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>

#include "utils.h"

int count_digits(int arg) {
    return snprintf(NULL, 0, "%d", arg);
}

void errormsg(const char const *msg, const char const *file, const int line) {
    fprintf(stderr, "[%s : %d] %s\n", file, line, msg);
}

char * safeAdvanceCharacters(const char *str, const size_t amt) {
    int i;
    for(i = 0; i < amt && *str; i++) str++;
    return (char *)str;
}

/*
    Builds a token from the given string until the specified delimiter is encountered
    or the end of the string is reached, depending on how usedelim is set. Memory for
    the token is dynamically allocated and must be freed by the user.
    Parameters:
        str - the string from which the token will be built
        delim - the delimiter that will terminate building of the token
        usedelim - determines whether or not delimiters will terminate token building
    Return:
        A dynamically allocated token if it is able to be built, or NULL if the token
        cannot be created for some reason.
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

char * intToStr(const int num) {
    char *result = malloc(MAX_INT_LENGTH);
    int ret = snprintf(result, MAX_INT_LENGTH, "%d", num);
    result = realloc(result, ret + 1);
    return result;
}

int writeToSocket(int sockfd, void *data, int len) {
    int totalSent = 0;
    int bytesSent = 0;
    while(len > 0) {
        bytesSent = write(sockfd, data, len);
        if(bytesSent < 1) {
            return bytesSent;
        }
        totalSent += bytesSent;
        data += bytesSent;
        len -= bytesSent;
    }
    return totalSent;
}

int readNBytes(int sockfd, void *data, int len) {
    int totalRead = 0;
    int bytesRead = 0;
    while(bytesRead < len) {
        bytesRead = read(sockfd, data, len);
        if(bytesRead < 1) {
            
        }
        totalRead += bytesRead;
        data += bytesRead;
        len  -= bytesRead;
    }
    return totalRead;
}

packet *packetCreate(void *data, header_size_t length) {
    packet *p = malloc(sizeof(packet));
    p->size = length;
    p->data = data;
    return p;
}

void packetDestroy(packet *p) {
    free(p->data);
    free(p);
}

/*
    Attempts to receive a packet in length prefix form. First, a number
    of bytes defined in HEADER_LENGTH are read from the socket. This is
    the length of the packet to come. Next, the packet is read from the 
    socket, stored in a char *, and null terminated then returned.
    Parameters:
        sockfd - the socket from which the packet will be read
    Return:
        A struct containing the packet data and size
*/
packet *readPacket(int sockfd) {
    /* READ SIZE PREFIX */
    header_size_t length = 0;
    int headres = readNBytes(sockfd, (void*)&length, HEADER_LENGTH);
    if(headres < 1) {
        //error
        return NULL;
    }
    length = ntohl(length);

    /* READ DATA */
    void *data = malloc(length);
    int datares = readNBytes(sockfd, data, length);
    if(datares < 1) {
        //error
        free(data);
        return NULL;
    }
    packet *pkt = packetCreate(data, length);
    return pkt;
}

/*
    Attempts to send a packet containing data to the given socket. Packets
    are sent in length prefix form.
    Parameters:
        sockfd - the socket to which the packet will be sent
        data - the data to be sent
        length - the length of the data in bytes
    Return:
        -1 or 0 on error, otherwise the number of bytes sent
*/
int sendPacket(int sockfd, packet *pkt) {
    uint32_t length = pkt->size;
    length = htonl(length);
    int headres = writeToSocket(sockfd, (void*)&length, HEADER_LENGTH);
    if(headres < 1) {
        //error
        return headres;
    }
    int packres = writeToSocket(sockfd, pkt->data, length);
    return packres;
}

char * errnoToCode(int eno) {
    switch(eno) {
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



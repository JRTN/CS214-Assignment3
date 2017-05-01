#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>

#include "utils.h"

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

int writeToSocket(int sockfd, char *data, int len) {
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

int readNBytes(int sockfd, char *data, int len) {
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

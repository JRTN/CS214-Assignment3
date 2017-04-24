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

int netopen(const char *pathname, int flags) {
    char buffer[256] = {0};
    printf("Enter a message: ");
    fgets(buffer, 255, stdin);
    int n = 0;
    if((n = write(sockfd, buffer, strlen(buffer))) < 0) {
        errormsg("ERROR writing to socket", __FILE__, __LINE__);
        return -1;
    }
    memset(buffer, 0, 256);
    if((n = read(sockfd, buffer, 255)) < 0) {
        errormsg("Error reading socket response", __FILE__, __LINE__);
        return -1;
    }
    printf("%s\n", buffer);
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

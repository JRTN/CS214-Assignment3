#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 40690
#define BACKLOG_SIZE 5

void errormsg(const char const * msg, const char const *file, const int line) {
    fprintf(stderr, "[%s : %d] %s\n", file, line, msg);
}

void * clientSockHandler(void * arg) {
    pthread_mutex_t socket_mutex;
    pthread_mutex_init(&socket_mutex, NULL);

    pthread_mutex_lock(&socket_mutex);
    int newsockfd = *((int*)arg);
    pthread_mutex_unlock(&socket_mutex);

    char buffer[256] = {0};
    int n = 0;
    if ( (n = read(newsockfd, buffer, 255)) < 0) {
        errormsg("ERROR reading from socket", __FILE__, __LINE__);
        close(newsockfd);
    }
    printf("%d bytes read\n", n);
    printf("%s\n", buffer);
    if ((n = write(newsockfd, "I got your message", 18)) < 0) {
        errormsg("ERROR writing to socket", __FILE__, __LINE__);
        close(newsockfd);
    }
    close(newsockfd);
    return 0;
}

/*
    Creates a new TCP IPv4 socket.
    Return:
        The file descriptor of the new socket if there are no errors, -1 otherwise
*/
static int createSocket() {
    int sockfd;
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
       errormsg("ERROR opening socket", __FILE__, __LINE__);
    }
    return sockfd;
}

/*
    Binds an existing socket file descriptor to the port defined by the macro PORT. The
    binding is set up to accept connections to all the IPs of the machine.
    Arguments:
        sockfd - the file descriptor of the socket that will be bound
    Return:
        0 if the socket is bound to the port successfully, -1 otherwise.
*/
static int bindSocket(int sockfd) {
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        errormsg("ERROR on binding", __FILE__, __LINE__);
        return -1;
    }
    return 0;
}

/*
    Accepts a new connection on the socket and returns the new socket file descriptor
    Arguments:
        sockfd - the file descriptor of the socket from which the connection will be accepted
    Return:
        The file descriptor of the socket which results from the call to accept, -1 otherwise
*/
static int acceptFromSocket(int sockfd) {
    struct sockaddr_in cli_addr = {0};
    socklen_t clilen = sizeof(cli_addr);
    int newsockfd;
    if((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0) {
        errormsg("ERROR on accept", __FILE__, __LINE__);
    }
    return newsockfd;
}

int main(void) {
    int sockfd = createSocket();
    bindSocket(sockfd);
    while(1) {
        listen(sockfd, BACKLOG_SIZE);
        int newsockfd = acceptFromSocket(sockfd);
        /* Place the new socket file descriptor on the heap so it can be used by the new thread */
        int *heapSocketfd = malloc(sizeof(int));
        *heapSocketfd = newsockfd;
        /* Create the new thread and pass it the new file descriptor */
        pthread_t connectionThread;
        int ptc = 0;
        if( (ptc = pthread_create(&connectionThread, NULL, clientSockHandler, (void*) heapSocketfd)) != 0) {
            errormsg("ERROR creating connection handling thread", __FILE__, __LINE__);
        }
    }
    return 0; 
}

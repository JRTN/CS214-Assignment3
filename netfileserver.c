#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 40690
#define BACKLOG_SIZE 5

void errormsg(const char const * msg, const char const *file, const int line) {
    fprintf(stderr, "[%s : %d] %s\n", file, line, msg);
}

void * clientSockHandler(void * arg) {
    int newsockfd = *(int*)arg;

    char buffer[256] = {0};
    int n = 0;
    if((n = read(newsockfd, buffer, 255) < 0)) {
        errormsg("ERROR reading from socket", __FILE__, __LINE__);
    } else {
        printf("%d bytes read\n", n);
        printf("%s\n", buffer);
    }
    close(newsockfd);
    free(arg);
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
    } else {
        printf("Socket (%d) successfully created\n", sockfd);
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
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        errormsg("ERROR on binding", __FILE__, __LINE__);
        return -1;
    } else {
        printf("Socket (%d) successfully bound to port %d\n", sockfd, PORT);
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
    } else {
        printf("New connection accepted on socket (%d), connection socket is (%d)\n", sockfd, newsockfd);
    }
    return newsockfd;
}

int main(void) {
    //Create a socket with the socket() system call
    int sockfd = createSocket();
    //Bind the socket to an address using the bind() system call
    bindSocket(sockfd);
    //Listen for connections with the listen() system call
    listen(sockfd, BACKLOG_SIZE);
    int clientSocket = 0;
    //Accept a connection with the accept() system call
    while( (clientSocket = acceptFromSocket(sockfd) )) {
        //Accept a connection with the accept() system call
        int *heapSocketfd = malloc(sizeof(int));
        *heapSocketfd = clientSocket;
        pthread_t connectionThread;
        int ptc = 0;
        if( (ptc = pthread_create(&connectionThread, NULL, clientSockHandler, (void*) heapSocketfd)) < 0) {
            errormsg("ERROR creating connection handling thread", __FILE__, __LINE__);
        } else {
            printf("Created handler thread for connection socket (%d)\n", clientSocket);
        }
    }
    return 0; 
}

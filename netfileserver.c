/*
    Basic idea:
    Main - Job is only to accept new connections
    - Creates socket
    - Binds socket to port
    - Listens on it
    - calls accept(serversocket)
        - when accept is called, new thread is called to handle connection
        - socket returned by accept is copied onto heap and accessed in thread
            newsock = accept(serversocket)
            sockptr = malloc(sizeof...)
            *sockptr = *newsock
        - Client Service Thread:
            (void*)clisockhandler(void *args)
            accesses socket through heap, can now talk to client
            read from socket and figure out what command is to be run: "open ./ O_W"
            parses read data and runs the command locally -> int result = open("./", O_W);
            check errno as needed
            write negative result to socket unless it's -1, then we have to return something else
            
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 40690
#define BACKLOG_SIZE 5

void errormsg(const char const * msg, const char const *file, const int line) {
    fprintf(stderr, "[%s : %d] %s\n", file, line, msg);
}

int main(void) {
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr = {0};
    struct sockaddr_in cli_addr = {0};
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
       errormsg("ERROR opening socket", __FILE__, __LINE__);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        errormsg("ERROR on binding", __FILE__, __LINE__);
    }
    while(1) {
        listen(sockfd, BACKLOG_SIZE);
        socklen_t clilen = sizeof(cli_addr);
        if( (newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0) {
            errormsg("ERROR on accept", __FILE__, __LINE__);
        }
        char buffer[256] = {0};
        int n = 0;
        if ( (n = read(newsockfd, buffer, 255)) < 0) {
            errormsg("ERROR reading from socket", __FILE__, __LINE__);
        }
        printf("Here is the message: %s\n",buffer);
        if ((n = write(newsockfd, "I got your message", 18)) < 0) {
            errormsg("ERROR writing to socket", __FILE__, __LINE__);
        }
    }
    close(newsockfd);
    close(sockfd);
    return 0; 
}

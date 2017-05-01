#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdbool.h>

#include "netfileserver.h"
#include "utils.h"
 
int getActualFileDes(int fildes) {
    if(fildes == NEG_ONE_FD) {
        return 1;
    } else {
        return -fildes;
    }
}

/*
    Builds and returns the response containing the results of a read operation that will
    be sent back to the client.
    Responses are of the form:
        <nbyte>!<buffer> 
    when there are no errors during the file operation, or
        -1!<error>
    when there is an error during the file operation
    Parameters:
        readbytes - the number of bytes that were read in the file operation
        buffer - the actual data that was read during the file operation
    Return:
        A formatted response that is to be sent to the client and interpreted client side
*/
char * buildReadResponse(ssize_t readbytes, char *buffer) {
    char * response = NULL;
    int readbytes_len = count_digits(readbytes);
    if(readbytes == -1) { //error reading
        char *errno_str = errnoToCode(errno);
        size_t errno_str_len = strlen(errno_str);
        int messagesize = errno_str_len + readbytes_len + 2;
        response = malloc(messagesize);
        sprintf(response, "%zd!%s", readbytes, errno_str);
        free(errno_str);
    } else {
        int messagesize = readbytes_len + readbytes + 2;
        response = malloc(messagesize);
        snprintf(response, messagesize, "%zd!%s", readbytes, buffer);
    }
    return response;
}

/*
    Performs a read file operation with the given parameters and passes the results to a
    function which formats these results to be sent back to the client. 
    Parameters:
        nbyte - A string representation of the number of bytes to be read
        filedes - A string representation of the file descriptor of the file to be read
    Return:
        A response as formatted in buildReadResponse()
*/
char * performReadOp(const char *nbyte, const char *filedes) {
    printf("Perform Read Op:\n");
    printf("nbyte: %s\n", nbyte);
    printf("filedes: %s\n", filedes);
    int n_nbyte = strtol(nbyte, NULL, 10);
    int n_filedes = strtol(filedes, NULL, 10);
    int actualFileDes = getActualFileDes(n_filedes);

    printf("nbyte: %d\n", n_nbyte);
    printf("filedes: %d\n", actualFileDes);

    char *buffer = malloc(n_nbyte + 1);
    buffer[n_nbyte] = '\0';

    ssize_t readbytes = read(actualFileDes, buffer, (size_t)n_nbyte);

    return buildReadResponse(readbytes, buffer);
}

/*
    Takes a client read message and parses the relevant information which is then passed
    to the performReadOp() function.
    Parameters:
        message - the client message containing the needed information
    Return:
        A response as formatted in buildReadResponse()
*/
char * parseReadMessage(const char *message) {
    char *nbyte = buildToken(message, DELIMITER, true);
    size_t adv = strlen(nbyte) + 1;
    message = safeAdvanceCharacters(message, adv);
    char *filedes = buildToken(message, DELIMITER, false);

    return performReadOp(nbyte, filedes);
}

/*
    Builds and returns the response containing the results of a write file operation
    Write responses are of the following form:
        <nbytes>
    if there are no errors during the write file operation, or
        -1!<error>
    if there is an error during the write file operation.
    Parameters:
        wrotebytes - the number of bytes written during the file operation
    Return:
        A formatted response that is to be sent to the client and interpreted client-side
*/
char * buildWriteResponse(ssize_t wrotebytes) {
    char *response = NULL;
    if(wrotebytes == -1) { //error writing to file
        char *errno_str = errnoToCode(errno);
        int messagesize = strlen(errno_str) + 4; //two for -1, one for delim, one for null
        response = malloc(messagesize);
        sprintf(response, "%d!%s", -1, errno_str);
        free(errno_str);
    } else {
        int wrotebytes_len = count_digits(wrotebytes);
        response = malloc(wrotebytes_len + 1);
        sprintf(response, "%zd", wrotebytes);
    }
    return response;
}

/*
    Performs a write file operation with the given parameters and passes the results to
    a function to be formatted for sending to the client.
    Parameters:
        filedes - string representation of the file to be written to
        nbyte - string representation of the numer of bytes to be written
        buffer - the bytes to be written
    Return:
        A formatted response as defined in buildWriteResponse()
*/
char * performWriteOp(const char *filedes, const char *nbyte, const char *buffer) {
    printf("Perform Write Op:\n");
    printf("filedes: %s\n", filedes);
    printf("nbyte: %s\n", nbyte);
    printf("buffer: %s\n", buffer);

    int n_nbyte = strtol(nbyte, NULL, 10);
    int n_filedes = strtol(filedes, NULL, 10);
    int actualFileDes = getActualFileDes(n_filedes);
    ssize_t wrotebytes = write(actualFileDes, (const void *)buffer, n_nbyte);
    
    return buildWriteResponse(wrotebytes);
}

/*
    Parses a client message for the information that is then sent to the performWriteOp()
    function.
    Parameters:
        message - the client message containing information for the file operation
    Return:
        A formatted response as defined in buildWriteResponse()
*/
char * parseWriteMessage(const char *message) {
    char *filedes = buildToken(message, DELIMITER, true);
    size_t adv = strlen(filedes) + 1;
    message = safeAdvanceCharacters(message, adv);
    char *nbyte = buildToken(message, DELIMITER, true);
    adv = strlen(nbyte) + 1;
    message = safeAdvanceCharacters(message, adv);
    char *buffer = buildToken(message, DELIMITER, false);

    return performWriteOp(filedes, nbyte, buffer);
}

/*
    Builds and returns the response containing the results of an open file operation.
    open responses are of the following form:
        <filedes>
    if there are no errors during the open file operation, or
        -1!<error>
    if there is an error during the open file operation.
    Parameters:
        filefd - the file descriptor returned from the call to open()
    Return:
        A formatted response that is to be sent to the client and interpreted client-side
*/
char * buildOpenResponse(int filefd) {
    char *response = NULL;
    if(filefd == -1) { //error opening file
        //get readable version of errno error code
        char *errno_str = errnoToCode(errno);
        //find length of readable error
        int fd_length = 2;
        size_t errno_str_len = strlen(errno_str);
        int messagesize = fd_length + errno_str_len + 2;
        response = malloc(messagesize);
        sprintf(response, "%d!%s", filefd, errno_str);
        free(errno_str);
    } else {
        if(filefd == 1) {
            filefd = NEG_ONE_FD;
        }
        filefd = -filefd;
        int fd_length = count_digits(filefd);
        response = malloc(fd_length + 1);
        sprintf(response, "%d", filefd);
    }
    return response;
}

/*
    Performs an open file operation with the given parameters and passes the results to
    a function to be formatted for sending to the client.
    Parameters:
        flag - a flag indicating how the file is to be opened
        pathname - the pathname of the file to be opened
    Return:
        A formatted response as defined in buildOpenResponse()
*/
char * performOpenOp(int flag, const char *pathname) {
    printf("Perform Open Op:\n");
    printf("Flag: %d\n", flag);
    printf("Pathname: %s\n", pathname);
    int filefd = open(pathname, flag);
    printf("filefd: %d\n", filefd);
    printf("flag: 0x%x\n", flag);
    
    return buildOpenResponse(filefd);
}

/*
    Parses a client message for the information that is then sent to the performOpenOp()
    function.
    Parameters:
        message - the client message containing information for the file operation
    Return:
        A formatted response as defined in buildOpenResponse()
*/
char * parseOpenMessage(const char *message) {
    int flag = *message - '0';
    message = safeAdvanceCharacters(message, 2);
    if(!*message) {
        //error, hit end of string before finding pathname
    }
    char *pathname = buildToken(message, DELIMITER, false);

    return performOpenOp(flag, pathname);
}

/*
    Builds and returns the response containing the results of a close file operation.
    close responses will always be the size defined by CLOSE_RESPONSE_SIZE, even though
    the actual information contained within may not need all bytes to be represented.
    close responses are of the following form:
        <size>!0
    if there are no errors during the close file operation, or
        <size>!-1!<error>
    if there is an error during the close file operation.
    Parameters:
        close_result - the value returned from the call to close()
    Return:
        A formatted response that is to be sent to the client and interpreted client-side
*/
char * buildCloseResponse(int close_result) {
    char *response = NULL;
    if(close_result == 0) { //closed successfully
        response = strdup("0");
    } else { //error closing
        char *errno_str = errnoToCode(errno);
        size_t errno_str_len = strlen(errno_str);
        response = malloc(errno_str_len + 4);
        sprintf(response, "-1!%s", errno_str);
        free(errno_str);
    }
    return response;
}

/*
    Performs a close file operation with the given parameters and passes the results to
    a function to be formatted for sending to the client.
    Parameters:
        filedes - string representation of the file descriptor of the file that will be closed
    Return:
        A formatted response as defined in buildCloseResponse()
*/
char * performCloseOp(const char *filedes) {
    printf("Perform Close Op:\n");
    printf("filedes: %s\n", filedes);
    int n_filedes = strtol(filedes, NULL, 10);
    int actualFileDes = getActualFileDes(n_filedes);
    int result = close(actualFileDes);
    return buildCloseResponse(result);
}

char * parseCloseMessage(const char *message) {
    char *filedes = buildToken(message, DELIMITER, false);

    return performCloseOp(filedes);
}

char * performInvalidOp(const char *message) {
    return strdup(MALFORMED_COMMAND_MSG);
}

/*
    Extracts the necessary information from a client message and calls the appropriate
    functions to perform the operations on the machine.
    Parameters:
        message - the message that will be parsed
*/
char * handleClientMessage(const char *message) {
    if(!message) return strdup(MALFORMED_COMMAND_MSG); //error
    printf("Operation: %c\n", *message);
    char operation = *message;
    //Advance to start of unique messages
    message = safeAdvanceCharacters(message, 2);
    switch(operation) {
        case OPEN_OP:  return parseOpenMessage(message);
        case READ_OP:  return parseReadMessage(message);
        case WRITE_OP: return parseWriteMessage(message);
        case CLOSE_OP: return parseCloseMessage(message);
        default:       return performInvalidOp(message); //invalid operation
    }
}

void * clientHandler(void * client) {
    int clientfd = *(int*)client;
    free(client);
    char *response = NULL;
    while(1) {
        packet *clientpkt = NULL;
        /* RECEIVE CLIENT PACKET */
        if( (clientpkt = readPacket(clientfd)) ) {
            printf("Packet Length: %d\n", clientpkt->size);
            /* HANDLE CLIENT MESSAGE */
            response = handleClientMessage((char *)clientpkt->data);
            packetDestroy(clientpkt);
            if(!response) break; //something went horribly wrong
            /* CONSTRUCT RESPONSE PACKET */
            uint32_t reslen = strlen(response);
            packet *responsepkt = packetCreate(response, reslen + 1);
            /* SEND RESPONSE PACKET BACK TO CLIENT */
            int response_res = sendPacket(clientfd, responsepkt);
            packetDestroy(responsepkt);
            if(response_res < 0) {
                break; //error
            }
        } else {
            break;
        }
    }
    close(clientfd);
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
    Binds an existing socket file descriptor to the port defined by the macro PORT.
    The binding is set up to accept connections to all the IPs of the machine.
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

int main(void) {
    //Create a socket with the socket() system call
    int sockfd = createSocket();
    //Bind the socket to an address using the bind() system call
    if(bindSocket(sockfd) == -1) {
        return EXIT_FAILURE; //error binding socket
    }
    //Listen for connections with the listen() system call
    listen(sockfd, BACKLOG_SIZE);
    //Accept a connection with the accept() system call
    while(1) {
        struct sockaddr_in client_addr = {0};
        socklen_t client_len;
        int newsocket_fd = accept(sockfd, (struct sockaddr *) &client_addr, &client_len);
        if(newsocket_fd < 0) {
            printf("Error on accept\n");
            return 0;
        } else {
            printf("Connection accepted on socket (%d)\n", sockfd);
            printf("Client socket fd: %d\n", newsocket_fd);
        }
        pthread_t thread;
        int *newsock = malloc(sizeof(int));
        *newsock = newsocket_fd;
        pthread_create(&thread, NULL, clientHandler, newsock);
        pthread_detach(thread);
    }
    return 0; 
}

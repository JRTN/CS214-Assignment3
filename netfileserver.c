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
    Gets the size of a message sent to the server. Every message always starts with the
    total size of the message.
    Parameters:
        message - the message which contains its size
    Return:
        The size of the message
*/
long int getMessageSize(const char const *message) {
    char *sizestr = buildToken(message, DELIMITER, true);
    long int result = strtol(sizestr, NULL, 10);
    free(sizestr);
    return result;
}

/*
    Builds and returns the response containing the results of a read operation that will
    be sent back to the client. Read responses will always at least be the size that is
    defined in READ_RESPONSE_BASE_SIZE, but they also may be larger depending on the
    size of the buffer which contains the data that was read.
    Responses are of the form:
        <size>!<nbyte>!<buffer> 
    when there are no errors during the file operation, or
        <size>!-1!<error>
    when there is an error during the file operation
    Parameters:
        readbytes - the number of bytes that were read in the file operation
        buffer - the actual data that was read during the file operation
    Return:
        A formatted response that is to be sent to the client and interpreted client side
*/
#define READ_RESPONSE_BASE_SIZE 30
char * buildReadResponse(ssize_t readbytes, char *buffer) {
    char * response = NULL;
    if(readbytes == -1) { //error readingq
        response = malloc(READ_RESPONSE_BASE_SIZE);
        char *errno_str = errnoToCode(errno);
        sprintf(response, "%d!%zd!%s", READ_RESPONSE_BASE_SIZE, readbytes, errno_str);
        free(errno_str);
    } else {
        size_t messagesize = readbytes + READ_RESPONSE_BASE_SIZE + 1;
        response = malloc(messagesize);
        sprintf(response, "%zd!%zd!%s", messagesize, readbytes, buffer);
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
    Builds and returns the response containing the results of a write file operation.
    Write responses will always be the size defined by WRITE_RESPONSE_SIZE, even though
    the actual information contained within may not need all bytes to be represented.
    Write responses are of the following form:
        <size>!<nbytes>
    if there are no errors during the write file operation, or
        <size>!-1!<error>
    if there is an error during the write file operation.
    Parameters:
        wrotebytes - the number of bytes written during the file operation
    Return:
        A formatted response that is to be sent to the client and interpreted client-side
*/
#define WRITE_RESPONSE_SIZE 30
char * buildWriteResponse(ssize_t wrotebytes) {
    char *response = malloc(WRITE_RESPONSE_SIZE);
    if(wrotebytes == -1) { //error writing to file
        char *errno_str = errnoToCode(errno);
        sprintf(response, "%d!%d!%s", WRITE_RESPONSE_SIZE, -1, errno_str);
        free(errno_str);
    } else {
        sprintf(response, "%d!%zd", WRITE_RESPONSE_SIZE, wrotebytes);
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
    Open responses will always be the size defined by OPEN_RESPONSE_SIZE, even though
    the actual information contained within may not need all bytes to be represented.
    open responses are of the following form:
        <size>!<filedes>
    if there are no errors during the open file operation, or
        <size>!-1!<error>
    if there is an error during the open file operation.
    Parameters:
        filefd - the file descriptor returned from the call to open()
    Return:
        A formatted response that is to be sent to the client and interpreted client-side
*/
#define OPEN_RESPONSE_SIZE 30
char * buildOpenResponse(int filefd) {
    char *response = malloc(OPEN_RESPONSE_SIZE);
    if(filefd == -1) { //error opening file
        //get readable version of errno error code
        char *errno_str = errnoToCode(errno);
        //find length of readable error
        sprintf(response, "%d!%d!%s", OPEN_RESPONSE_SIZE, filefd, errno_str);
        free(errno_str);
    } else {
        if(filefd == 1) {
            filefd = NEG_ONE_FD;
        }
        sprintf(response, "%d!%d", OPEN_RESPONSE_SIZE, -filefd);
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
#define CLOSE_RESPONSE_SIZE 30
char * buildCloseResponse(int close_result) {
    char *response = malloc(CLOSE_RESPONSE_SIZE);
    if(close_result == 0) { //closed successfully
        sprintf(response, "%d!%d", CLOSE_RESPONSE_SIZE, close_result);
    } else { //error closing
        char *errno_str = errnoToCode(errno);
        sprintf(response, "%d!%d!%s", CLOSE_RESPONSE_SIZE, close_result, errno_str);
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
    thread_info_t* client_data = (thread_info_t*) client;
    #define clientfd client_data->client_fd
    char *response = NULL;
    while(1) {
        packet *pkt = NULL;
        if( (pkt = readPacket(clientfd)) ) {
            header_size_t pktsze = pkt->size;
            printf("Packet Length: %d\n", pktsze);
            response = handleClientMessage((char *)pkt->data);
            if(!response) break; //something went horribly wrong
            uint32_t responselen = strlen(response);
            //if(write(clientfd, response, strlen(response) + 1) < 0) {
            if(sendPacket(clientfd, (void*) response, responselen) < 0) {
                break;
            }
            free(response);
        } else {
            break;
        }
    }
    write(clientfd, END_CONN_MSG, END_CONN_MSG_LEN);
    sendPacket(clientfd, END_CONN_MSG, END_CONN_MSG_LEN);
    close(clientfd);
    #undef clientfd
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
    pthread_t * threads = malloc(sizeof(pthread_t) * THREAD_LIMIT);
    thread_info_t * thread_info = malloc(sizeof(thread_info_t) * THREAD_LIMIT);
    int client_count = -1;
    //Accept a connection with the accept() system call
    while(1) {
        client_count++;
        thread_info[client_count].client_fd = accept(sockfd, (struct sockaddr*) &(thread_info[client_count].client_addr),
                                                     &thread_info[client_count].client_addr_len);
        if(thread_info[client_count].client_fd < 0) {
            return 0;
        } else {
            printf("Connection accepted on socket (%d)\n", sockfd);
            printf("Client socket fd: %d\n", thread_info[client_count].client_fd);
        }
        int ptc = pthread_create(&threads[client_count], NULL, clientHandler, (void*) &thread_info[client_count]);
        if(ptc < 0) {
            //error
        }
    }
    return 0; 
}

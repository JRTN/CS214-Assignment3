#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define PORT 40690
#define BACKLOG_SIZE 5
#define THREAD_LIMIT 5
#define CHUNK_SIZE 5
#define DELIMITER '!'
#define MAX_INT_LENGTH 11

#define READ_OP 'r'
#define WRITE_OP 'w'
#define OPEN_OP 'o'
#define CLOSE_OP 'c'

typedef struct thread_info_t{
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
} thread_info_t;

void errormsg(const char const *msg, const char const *file, const int line) {
    fprintf(stderr, "[%s : %d] %s\n", file, line, msg);
}

char * safeAdvanceCharacters(const char *str, const size_t amt) {
    int i;
    for(i = 0; i < amt && *str; i++) str++;
    return (char *)str;
}

char * intToStr(const int num) {
    char *result = malloc(MAX_INT_LENGTH);
    int ret = snprintf(result, MAX_INT_LENGTH, "%d", num);
    result = realloc(result, ret + 1);
    return result;
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

char * performReadOp(const char *nbyte, const char *filedes) {
    printf("Perform Read Op:\n");
    printf("nbyte: %s\n", nbyte);
    printf("filedes: %s\n", filedes);

    return NULL;
}

char * parseReadMessage(const char *message) {
    char *nbyte = buildToken(message, DELIMITER, true);
    size_t adv = strlen(nbyte) + 1;
    message = safeAdvanceCharacters(message, adv);
    char *filedes = buildToken(message, DELIMITER, false);

    return performReadOp(nbyte, filedes);;
}

char * performWriteOp(const char *filedes, const char *nbyte, const char *buffer) {
    printf("Perform Write Op:\n");
    printf("filedes: %s\n", filedes);
    printf("nbyte: %s\n", nbyte);
    printf("buffer: %s\n", buffer);

    return NULL;
}

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

char * performOpenOp(int flag, const char *pathname) {
    printf("Perform Open Op:\n");
    printf("Flag: %d\n", flag);
    printf("Pathname: %s\n", pathname);
    return NULL;
}

char * parseOpenMessage(const char *message) {
    int flag = *message - '0';
    message = safeAdvanceCharacters(message, 2);
    char *pathname = buildToken(message, DELIMITER, false);

    return performOpenOp(flag, pathname);
}

char * performCloseOp(const char *filedes) {
    printf("Perform Close Op:\n");
    printf("filedes: %s\n", filedes);

    return NULL;
}

char * parseCloseMessage(const char *message) {
    char *filedes = buildToken(message, DELIMITER, false);

    return performCloseOp(filedes);
}

char * performInvalidOp(const char *message) {
    printf("Perform Invalid Op\n");
    return NULL;
}

/*
    Extracts the necessary information from a client message and calls the appropriate
    functions to perform the operations on the machine.
    Parameters:
        message - the message that will be parsed
*/
char * handleClientMessage(const char *message) {
    if(!message) return NULL; //error
    //Advance to first delimiter
    while(*message && *message != DELIMITER) message++;
    //Advance to character after delimiter
    message = safeAdvanceCharacters(message, 1);
    if(!*message) return NULL; //wasn't able to advance two characters before hitting end of string
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
    char buffer[4096] = {0};
    char *response = NULL;
    int readbytes = 0;
    while(1) {
        if( (readbytes = read(clientfd, &buffer, 4096)) > 0) {
            long int expectedsize = getMessageSize(buffer);
            printf("Client (%d) message: %s\n", clientfd, buffer);
            printf("Read Bytes: %d Expected Bytes: %ld\n", readbytes, expectedsize);
            response = handleClientMessage(buffer);
            response = "I got your message";
            if(write(clientfd, response, strlen(response)) < 0) {
                //error
            }
        } else {
            //error receiving message from client
        }
    }

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

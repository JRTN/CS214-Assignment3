#ifndef netfileserver_h
#define netfileserver_h

#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

#define OUR_O_RDONLY 1
#define OUR_O_WRONLY 2
#define OUR_O_RDWR   3

#define NEG_ONE_FD 100000

#define MALFORMED_COMMAND_MSG "11!EMALCMD"

#define END_CONN_MSG "12!END_CONN"
#define END_CONN_MSG_LEN 12

typedef struct thread_info_t{
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
} thread_info_t;

#endif

#ifndef libnetfiles_h
#define libnetfiles_h

#include <unistd.h>

/* OPEN FLAGS */
#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR   3

#define READ 1
#define WRITE 2


/* ERROR CODES 
#define EACCES             1
#define EINTR              2
#define EISDIR             3
#define ENOENT             4
#define EROFS              5
#define ENFILE             6
#define EWOULDBLOCK        7
#define EPERM              8
#define ETIMEDOUT          9
#define EBADF             10
#define ECONNRESET        11
#define HOST_NOT_FOUND    13
#define INVALID_FILE_MODE 14*/

#define PORT 40690

#define h_addr h_addr_list[0]

ssize_t netread(int, void *, size_t);
ssize_t netwrite(int, const void *, size_t);

int netopen(const char *, int flags);
int netclose(int);
int netserverinit(char *);

#endif

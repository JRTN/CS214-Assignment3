#ifndef libnetfiles_h
#define libnetfiles_h

#include <unistd.h>

/* OPEN FLAGS */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2

#define READ 1
#define WRITE 2
#define DELIMITER '!'

#define PORT 40690

#define h_addr h_addr_list[0]

#define HOST_NOT_FOUND 100

ssize_t netread(int, void *, size_t);
ssize_t netwrite(int, const void *, size_t);

int netopen(const char *, int flags);
int netclose(int);
int netserverinit(char *);

#endif

#include "libnetfiles.h"

#include <stdio.h>

int main(int argc, char **argv) {
    if(argc < 2) {
        fprintf(stderr, "Need host name\n");
        return 0;
    }
    int check = netserverinit(argv[1]);
    printf("returns %d from trying to connect to %s\n",check, argv[1]);
    if(check == -1){
        return -1;
    }
    int filed = netopen("/Documents/SysPrg/CS214-Assignment3/hi.txt", 0);
    printf("file descriptor: %d\n", filed);
    //char buffer[256] = {0};
    netclose(filed);
    close(filed);
    return 1;
}

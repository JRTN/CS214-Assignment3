#include "libnetfiles.h"

#include <stdio.h>

int main(int argc, char **argv) {
    if(argc < 2) {
        fprintf(stderr, "Need host name\n");
        return 0;
    }
    netserverinit(argv[1]);
    int filed = netopen("~/Desktop/Some/Directory", 0);
    char buffer[256] = {0};
    netread(filed, buffer, 255);
}

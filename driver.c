#include "libnetfiles.h"

#include <stdio.h>

int main(int argc, char **argv) {
    if(argc < 2) {
        fprintf(stderr, "Need host name\n");
        return 0;
    }
    netserverinit(argv[1]);
    netopen("temp", 0);
    netopen("two", 2);
    netopen("three", 3);
}

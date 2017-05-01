#ifndef utils_h
#define utils_h

#include <stdlib.h>
#include <stdbool.h>

#define CHUNK_SIZE 5
#define MAX_INT_LENGTH 11
#define HEADER_LENGTH 4

typedef uint32_t header_size_t;

typedef struct packet_t {
    header_size_t size;
    void *data;
} packet;

void errormsg(const char const *, const char const *, const int);
char *safeAdvanceCharacters(const char *, const size_t);
char *intToStr(const int);
int writeToSocket(int, void *, int);
int readNBytes(int, void *, int);
char *errnoToCode(int);
char *buildToken(const char *, const char, bool);
packet *readPacket(int);
int sendPacket(int, packet *);
void packetDestroy(packet *);
packet *packetCreate(void *, header_size_t);
int count_digits(int);

#endif

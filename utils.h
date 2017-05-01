#ifndef utils_h
#define utils_h

#include <stdlib.h>
#include <stdbool.h>

#define CHUNK_SIZE 5
#define MAX_INT_LENGTH 11
#define HEADER_LENGTH 4

typedef uint32_t header_size_t;

void errormsg(const char const *, const char const *, const int);
char *safeAdvanceCharacters(const char *, const size_t);
char *intToStr(const int);
int writeToSocket(int, void *, int);
int readNBytes(int, void *, int);
char *errnoToCode(int);
char *buildToken(const char *, const char, bool);
char *readPacket(int);
int sendPacket(int, void *, header_size_t);

#endif

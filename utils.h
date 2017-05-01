#ifndef utils_h
#define utils_h

#include <stdlib.h>
#include <stdbool.h>

#define CHUNK_SIZE 5
#define MAX_INT_LENGTH 11

void errormsg(const char const *, const char const *, const int);
char *safeAdvanceCharacters(const char *, const size_t);
char *intToStr(const int);
int writeToSocket(int, char *, int);
int readNBytes(int, char *, int);
char *errnoToCode(int);
char *buildToken(const char *, const char, bool);

#endif

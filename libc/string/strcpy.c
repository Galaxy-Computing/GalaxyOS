#include <string.h>

char *strcpy(char *dest, const char *src) {
    char *ptr = dest;
    while ((*dest++ = *src++) != '\0') {}
    return ptr;
}
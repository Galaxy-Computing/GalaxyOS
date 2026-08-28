#include <string.h>

char *strncat(char *dest, const char *src, size_t n) {
    char *ret = dest;

    while (*dest) { dest++; }
    while (n-- && (*dest++ = *src++)) {}
    if (n == (size_t)-1) { *dest = '\0'; }
    return ret;
}
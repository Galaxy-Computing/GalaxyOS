#include <string.h>

char *strcat(char *dest, const char *src) {
    char *ptr = dest;
    
    while (*ptr) ptr++;
    while ((*ptr++ = *src++));
    
    return dest;
}
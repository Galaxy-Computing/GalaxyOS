#include <string.h>

char *strrchr(register const char *s, int c) {
    char *retval = 0;

    do {
        if (*s == c)
            retval = (char*) s;
    } while (*s++);
    return retval;
}
#include <string.h>

char *strncpy (char *s1, const char *s2, size_t n) {
    size_t size = strlen(s2);
    if (size > n) size = n;
    if (size != n) memset(s1 + size, '\0', n - size);
    return memcpy(s1, s2, size);
}
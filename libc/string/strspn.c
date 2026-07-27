#include <string.h>
#include <stddef.h>
#include <limits.h>

size_t strspn(const char *s, const char *accept) {
    const char *start = s;
    
    unsigned char table[UCHAR_MAX + 1] = {0};
    
    if (!accept[0]) {
        return 0;
    }
    
    if (!accept[1]) {
        while (*s == *accept) {
            s++;
        }
        return s - start;
    }
    
    while (*accept) {
        table[(unsigned char)*accept++] = 1;
    }
    
    while (*s && table[(unsigned char)*s]) {
        s++;
    }
    
    return s - start;
}

size_t strcspn(const char *s1, const char *s2) {
    unsigned char rejected_map[UCHAR_MAX + 1] = {0};
    size_t count = 0;

    while (*s2 != '\0') {
        rejected_map[(unsigned char)*s2] = 1;
        s2++;
    }

    while (s1[count] != '\0') {
        if (rejected_map[(unsigned char)s1[count]]) {
            break;
        }
        count++;
    }

    return count;
}
#include <stddef.h>
#include <stdint.h>
#include <string.h>

void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = (const unsigned char *)s;
    unsigned char target = (unsigned char)c;

    while (((uintptr_t)p & (sizeof(size_t) - 1)) != 0) {
        if (n == 0) return NULL;
        if (*p == target) return (void *)p;
        p++;
        n--;
    }

    if (n >= sizeof(size_t)) {
        size_t mask = 0;
        for (size_t i = 0; i < sizeof(size_t); i++) {
            mask = (mask << 8) | target;
        }

        size_t high_bits = ((size_t)-1 / 0xff) * 0x80;
        size_t low_bits  = ((size_t)-1 / 0xff);

        const size_t *word_ptr = (const size_t *)p;

        while (n >= sizeof(size_t)) {
            size_t word = *word_ptr ^ mask;
            
            if (((word - low_bits) & ~word & high_bits) != 0) {
                break; 
            }
            word_ptr++;
            n -= sizeof(size_t);
        }
        p = (const unsigned char *)word_ptr;
    }

    while (n > 0) {
        if (*p == target) return (void *)p;
        p++;
        n--;
    }

    return NULL;
}

void *__rawmemchr (const void *s, int c) {
  if ((unsigned char) c != '\0') return memchr(s, c, (size_t)-1);
  return (char *)s + strlen(s);
}
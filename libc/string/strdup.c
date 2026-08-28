#include <stdlib.h>
#include <string.h>

#if defined(__is_libk)
#include <kernel/liballoc.h>
#endif

char *strdup(const char *s) {
    size_t len = strlen(s) + 1;

    #if defined(__is_libk)
    void *new = kmalloc(len);
    #else
    void *new = malloc(len);
    #endif

    if (new == NULL)
        return NULL;

    return (char*)memcpy(new, s, len);
}
#include <stdio.h>

#if defined(__is_libk)
#include <kernel/term.h>
#else
#include <unistd.h>
#endif

int putchar(int ic) {
#if defined(__is_libk)
    char c = (char) ic;
    term_write(&c, sizeof(c));
#else
    char c = (char) ic;
    write(1, &c, 1);
#endif
    return ic;
}

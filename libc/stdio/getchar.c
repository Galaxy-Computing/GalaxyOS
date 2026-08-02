#include <stdio.h>
#if defined(__is_libk)
#include <kernel/term.h>
#endif

int getchar() {
#if defined(__is_libk)
    char tchar;
    size_t chars_read = term_read(&tchar, 1);
    return (int)tchar;
#else
    return 0;
#endif
}
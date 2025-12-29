#include <stdio.h>
#if defined(__is_libk)
#include <kernel/keybd.h>
#endif

int getchar() {
#if defined(__is_libk)
    
#else
    // TODO: Implement stdio and the write system call.
#endif
}
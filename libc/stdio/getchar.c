#include <stdio.h>
#include <kernel/keybd.h>

int getchar() {
#if defined(__is_libk)
    
#else
    // TODO: Implement stdio and the write system call.
#endif
}
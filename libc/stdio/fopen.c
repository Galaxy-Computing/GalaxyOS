#include <stdio.h>

#if defined(__is_libk)
#include <kernel/vfs.h>
#endif

FILE *fopen(const char *filename, const char *mode) {
    #if defined(__is_libk)
    return vfs_get_file(filename, mode);
    #else
    
    #endif
}
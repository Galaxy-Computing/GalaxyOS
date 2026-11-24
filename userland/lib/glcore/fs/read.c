#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <fs.h>

void* fsReadAll(char* path, size_t* outsize) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) { return NULL; }
    int size = fsSize(fd);
    if (size < 0) { return NULL; }
    void *data = malloc(size);
    int retval = read(fd, data, size);
    if (retval < 0) { return NULL; }
    close(fd);

    *outsize = size;
    return data;
}
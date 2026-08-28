#include <stdio.h>
#include <fs.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
    if (argc > 1) {
        int fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            printf("cat: failed to open file\n");
            return -1;
        }
        int size = fsSize(fd);
        char *data = malloc(size + 1);
        read(fd, data, size);
        close(fd);
        data[size] = '\0';
        printf(data);
        putchar('\n');
    } else {
        printf("cat: not enough arguments\n");
        return -1;
    }
    return 0;
}
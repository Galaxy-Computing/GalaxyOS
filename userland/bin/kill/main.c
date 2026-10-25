#include <ps.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("kill: not enough arguments\n");
        return 0;
    }
    int retvalue = psKill(atoi(argv[1]));
    if (retvalue == -ESRCH) { printf("kill: no such process\n"); }
    if (retvalue == -EINVAL) { printf("kill: invalid argument\n"); }
    if (retvalue == -EACCES) { printf("kill: no permission\n"); }
    return retvalue;
}
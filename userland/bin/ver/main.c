#include <sysbase.h>
#include <stdio.h>

char verbuf[64];

int main(void) {
    sysVersionN(verbuf, 64);
    printf("%s\n", verbuf);
    return 0;
}
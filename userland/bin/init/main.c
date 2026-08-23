#include <stdio.h>

int main(void) {
    printf("Hello from userspace!");
    for (;;) { } // this is not a good way to hang here, but i don't care
}
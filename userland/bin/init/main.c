#include <stdio.h>
#include <ps.h>

#ifdef __i386__
#define SHELL_BINARY "local:galaxyos/i386/gsh.elf"
#endif

int main(void) {
    printf("Hello from userspace!\n");
    int shellpid = psExecA(SHELL_BINARY, NULL);
    if (shellpid == -1) printf("psExecA() failed\n"); // execute the shell binary
    psWait(shellpid);
}
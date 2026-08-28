#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <ps.h>
#include <fs.h>
#include <sysbase.h>

#define CONFIG_CMDLINE "local:galaxyos/config/cmdline"
#define CONFIG_PATH "local:galaxyos/config/path"
#define DEFAULT_PWD "local:"

int main(int argc, char* argv[]) {
    if (argc != 1) {
        printf("init must be ran by the kernel\n");
        return -1;
    }
    if (argv[0][0] != '\0') {
        printf("init must be ran by the kernel\n");
        return -1;
    }

    char *verbuf = malloc(64);
    sysVersionN(verbuf, 64);
    printf("\nWelcome to %s\n\n", verbuf);

    chdir(DEFAULT_PWD);

    int fdcmdline = open(CONFIG_CMDLINE, O_RDONLY);
    if (fdcmdline < 0) { return -1; }
    int sizesh = fsSize(fdcmdline);
    char *shpath = malloc(sizesh + 1);
    read(fdcmdline, shpath, sizesh);
    close(fdcmdline);
    shpath[sizesh] = '\0';

    int fdpath = open(CONFIG_PATH, O_RDONLY);
    if (fdpath < 0) { return -1; }
    int sizepath = fsSize(fdpath);
    char *pathvar = malloc(sizepath + 1);
    read(fdpath, pathvar, sizepath);
    close(fdpath);
    pathvar[sizepath] = '\0';

    printf("init: setting path variable\n");
    psSetEVar("PATH", pathvar, sizepath);

    printf("init: starting shell binary\n");
    for (;;) {
        int shellpid = psExecA(shpath, (char *[]){shpath, NULL});
        if (shellpid == -1) printf("psExecA() failed\n"); // execute the shell binary
        psWait(shellpid);
        printf("init: shell died, starting it again\n");
    }
}
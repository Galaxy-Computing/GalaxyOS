#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <ps.h>
#include <fs.h>
#include <sysbase.h>

#include "init.h"

#define CONFIG_CMDLINE "local:galaxyos/config/cmdline"
#define CONFIG_PATH "local:galaxyos/config/path"
#define DEFAULT_PWD "local:"

int teardown = 0;

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
    psSetEVar("PATH", pathvar);

    printf("init: starting confsrv\n");
    psExecA("local:galaxyos/i386/confsrv.elf", (char *[]){"confsrv.elf", NULL});

    printf("init: starting power management thread\n");
    int newtid = psThreadFork();
    if (newtid == 0) {
        // child thread
        powerthread();

        // should never be down here
        printf("init: power management thread returned!!\n");
        psExitThread();
    }

    printf("init: starting shell binary\n");
    for (;;) {
        // execute the shell binary
        int shellpid = psExecA(shpath, (char *[]){shpath, NULL});
        if (shellpid == -1) printf("psExecA() failed\n");
        psWait(shellpid);
        if (teardown == 1) {
            psExitThread();
            for (;;) {} // fallback if we're somehow down here
        }
        printf("init: shell died, starting it again\n");
    }
}
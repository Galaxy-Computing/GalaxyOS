#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <ps.h>
#include <fs.h>
#include <sysbase.h>
#include <confapi.h>
#include <errno.h>

#include "init.h"

#define DEFAULT_PWD "local:"
#define MAX_CFGVAR_LENGTH 512

int teardown = 0;

int main(int argc, char* argv[]) {
    int userran = 0;
    if (argc != 1) { userran = 1; }
    if (argv[0][0] != '\0') { userran = 1; }
    if (userran) {
        printf("init must be ran by the kernel\n");
        return -1;
    }

    char *verbuf = malloc(64);
    sysVersionN(verbuf, 64);
    printf("\nWelcome to %s\n\n", verbuf);

    chdir(DEFAULT_PWD);

    /*
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
    */

    psInterfaceCreate("sys:panic", 1); // make sure this interface exists before we start confsrv in order to avoid potential race condition

    printf("init: starting power management threads\n");
    int newtid = psThreadFork();
    if (newtid == 0) {
        // child thread
        powerthread();

        // should never be down here
        printf("init: power management thread returned!!\n");
        psExitThread();
    }

    newtid = psThreadFork();
    if (newtid == 0) {
        // child thread
        panicthread();

        // should never be down here
        printf("init: panic thread returned!!\n");
        psExitThread();
    }

    printf("init: starting confsrv\n");
    int retval = psExecA("local:galaxyos/i386/confsrv.elf", (char *[]){"confsrv.elf", NULL});
    if (retval == -ENOENT) { sysKernelPanic("System file missing or corrupt"); }
    
    char pathvar[MAX_CFGVAR_LENGTH];
    char shpath[MAX_CFGVAR_LENGTH];

    retval = confapiGetValue("init.path", pathvar, MAX_CFGVAR_LENGTH);
    while (retval == -ENOENT) { retval = confapiGetValue("init.path", pathvar, MAX_CFGVAR_LENGTH); }
    confapiGetValue("init.cmdline", shpath, MAX_CFGVAR_LENGTH);

    // prevent buffer overrun
    pathvar[MAX_CFGVAR_LENGTH-1] = '\0';
    shpath[MAX_CFGVAR_LENGTH-1] = '\0';

    printf("init: setting path variable\n");
    psSetEVar("PATH", pathvar);

    printf("init: starting shell binary\n");
    for (;;) {
        // execute the shell binary
        int shellpid = psExecA(shpath, (char *[]){shpath, NULL});
        if (shellpid == -1) printf("psExecA() failed\n");
        psWait(shellpid);
        if (teardown == 1) {
            psExitThread();
            for (;;) { psYield(); } // fallback if we're somehow down here
        }
        printf("init: shell died, starting it again\n");
    }
}
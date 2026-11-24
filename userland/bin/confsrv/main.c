#include <gcf.h>
#include <ps.h>
#include <stdio.h>
#include <fs.h>
#include <errno.h>
#include <stdlib.h>

#include "comm.h"

#define SYSTEM_CONFIG_PATH "local:galaxyos/config/system.gcf"

void *gcfdata;
size_t sizegcf = 0;

int main(void) {
    // create interfaces
    int retval = psInterfaceCreate("sys:conf.set", 1);
    if (retval == -EEXIST) {
        printf("confsrv already running\n");
        return 0;
    }
    psInterfaceCreate("sys:conf.get", 1);

    // load config
    gcfdata = fsReadAll(SYSTEM_CONFIG_PATH, &sizegcf);
    if (gcfdata == NULL) {
        psInterfaceSend("sys:panic", 0, "confsrv.elf: Couldn't read system configuration", 48);
        return 1;
    }

    if (gcf_is_valid(gcfdata, sizegcf)) {
        psInterfaceSend("sys:panic", 0, "confsrv.elf: Invalid system configuration", 42);
        return 1;
    }

    gcfdata = realloc(gcfdata, sizegcf + 2048);

    int newtid = psThreadFork();
    if (newtid == 0) {
        // child thread
        getthread();

        // should never be down here
        psExitThread();
    }

    newtid = psThreadFork();
    if (newtid == 0) {
        // child thread
        setthread();

        // should never be down here
        psExitThread();
    }
    
    psExitThread();
}
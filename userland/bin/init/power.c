#include <ps.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sysbase.h>
#include "init.h"

char buf[256];

int reboot(int sender) {
    printf("init: rebooting now\n");
    teardown = 1;
    int processes = psCount();
    int list[processes];
    psList(list, processes);

    // make sure the process waiting for a response doesn't wait forever
    int ret = 0;
    psInterfaceSend("sys:power", sender, &ret, 1);

    for (int i = 0; i < processes; i++) {
        if (list[i] > 2) {
            printf("killed %i\n", list[i]);
            psKill(list[i]);
        }
    }

    while (psCount() != 3) {} // wait for everything to be properly cleaned up

    return sysSetPowerState(sysPS_REBOOT);
}

void powerthread(void) {
    psInterfaceCreate("sys:power", 1);
    for (;;) {
        size_t sizeread = 0;
        int sender = psInterfaceRecieve("sys:power", buf, 256, &sizeread);
        if (sender < 0) continue;
        int ret = 0;
        if (!strncmp(buf, "shutdown", 256)) {
            ret = -ENOSYS;
            psInterfaceSend("sys:power", sender, &ret, 1);
        } else if (!strncmp(buf, "reboot", 256)) {
            reboot(sender);
        } else {
            ret = -ENOSYS;
            psInterfaceSend("sys:power", sender, &ret, 1);
        }
        
    }
}


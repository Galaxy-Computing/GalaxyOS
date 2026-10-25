#include <ps.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sysbase.h>

char buf[256];

int reboot(void) {
    printf("init: rebooting now\n");
    int processes = psCount();
    int list[processes];
    psList(list, processes);

    for (int i = 0; i < processes; i++) {
        if (list[i] > 2) {
            printf("killed %i\n", list[i]);
            psKill(list[i]);
        }
    }

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
        } else if (!strncmp(buf, "reboot", 256)) {
            ret = reboot();
        } else {
            ret = -ENOSYS;
        }
        psInterfaceSend("sys:power", sender, &ret, 1);
    }
}


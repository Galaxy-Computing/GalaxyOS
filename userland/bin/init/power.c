#include <ps.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sysbase.h>

char buf[256];

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
            printf("init: rebooting now\n");
            ret = sysSetPowerState(sysPS_REBOOT);
        } else {
            ret = -ENOSYS;
        }
        psInterfaceSend("sys:power", sender, &ret, 1);
    }
}
#include <usercom/power.h>
#include <ps.h>

int powerShutdown(void) {
    int rval = psInterfaceSend("sys:power", 0, "shutdown", 9);
    if (rval < 0) return rval;

    int comret = 0;
    size_t sizeread = 0;
    rval = psInterfaceRecieve("sys:power", &comret, 1, &sizeread);
    if (rval < 0) return rval;

    return comret;
}

int powerReboot(void) {
    int rval = psInterfaceSend("sys:power", 0, "reboot", 7);
    if (rval < 0) return rval;

    int comret = 0;
    size_t sizeread = 0;
    rval = psInterfaceRecieve("sys:power", &comret, 1, &sizeread);
    if (rval < 0) return rval;

    return comret;
}
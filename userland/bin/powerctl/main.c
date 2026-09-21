#include <stdio.h>
#include <string.h>
#include <ps.h>
#include <usercom/power.h>
#include <errno.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("usage: powerctl <command>\n");
        printf("valid commands:");
        printf("  shutdown - power off machine");
        printf("  reboot   - reboot machine");
        return 0;
    }
    if (!strcmp(argv[1], "shutdown")) {
        int retval = powerShutdown();
        if (retval == -ENOENT) {
            printf("powerctl: shutdown operation failed, interface does not exist\n");
        }
    } else if (!strcmp(argv[1], "reboot")) {
        int retval = powerReboot();
        if (retval == -ENOENT) {
            printf("powerctl: reboot operation failed, interface does not exist\n");
        }
    }
    return 0;
}
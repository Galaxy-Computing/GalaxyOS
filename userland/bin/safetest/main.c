#include <ps.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sysbase.h>
#include <fcntl.h>
#include <fs.h>
#include <string.h>
#include <stdlib.h>

void* ptr = NULL;

int main(void) {
    int retval = 0;
    int success = 0;
    int fail = 0;
    int test = 0;

    printf("read(): ");
    test++;
    retval = (int)read(0, ptr, 100);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("write(): ");
    test++;
    retval = (int)write(1, ptr, 100);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("open(): ");
    test++;
    errno = 0;
    open(ptr, 0);
    if (errno == EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("chdir(): ");
    test++;
    retval = chdir(ptr);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("getcwd(): ");
    test++;
    retval = (int)getcwd(ptr, 100);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psGetEVar(): ");
    test++;
    retval = psGetEVar(ptr, ptr, 100);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psSetEVar(): ");
    test++;
    retval = psSetEVar(ptr, ptr);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psInterfaceCreate(): ");
    test++;
    retval = psInterfaceCreate(ptr, 0);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psInterfaceDelete(): ");
    test++;
    retval = psInterfaceDelete(ptr);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psInterfaceSend(): ");
    test++;
    retval = psInterfaceSend(ptr, -1, ptr, 10);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psInterfaceRecieve(): ");
    test++;
    retval = psInterfaceRecieve(ptr, ptr, 100, ptr);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psInterfaceWait(): ");
    test++;
    retval = psInterfaceWait(ptr);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("fsList() invalid path: ");
    test++;
    retval = fsList(ptr, (char*)ptr, 100, 1);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("fsList() invalid buffer: ");
    test++;
    retval = fsList("local:", (char*)ptr, 100, 1);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("fsList() kernel buffer: ");
    test++;
    retval = fsList("local:", (char*)0xC0000000, 100, 1);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("fsList() wrapping buffer: ");
    test++;
    retval = fsList("local:", (char*)0xBFFFFFFF, 2, 1);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("fsList() zero length: ");
    test++;
    retval = fsList("local:", (char*)ptr, 0, 1);
    if (retval >= 0) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("fsList() truncation: ");
    test++;
    unsigned char canary[16];
    memset(canary, 0xAA, sizeof(canary));
    retval = fsList("local:", (char*)canary, 4, 1);
    int intact = (retval >= 0);
    for (size_t i = 4; i < sizeof(canary); i++) {
        if (canary[i] != 0xAA) { intact = 0; }
    }
    if (intact) {
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("sysVersion(): ");
    test++;
    retval = sysVersion(ptr);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("sysVersion() kernel buffer: ");
    test++;
    retval = sysVersionN((char*)0xC0000000, 100);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("getcwd() kernel buffer: ");
    test++;
    retval = (int)getcwd((char*)0xC0000000, 100);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("close() negative fd: ");
    test++;
    retval = close(0);
    if (retval == -EBADF) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("fsSize() negative fd: ");
    test++;
    retval = fsSize(0);
    if (retval == -EBADF) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psWait() out of range: ");
    test++;
    retval = psWait(0x7FFFFFFF);
    if (retval < 0) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psKill() out of range: ");
    test++;
    retval = psKill(0x7FFFFFFF);
    if (retval < 0) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psList() kernel buffer: ");
    test++;
    retval = psList((int*)0xC0000000, 10);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("brk() kernel address: ");
    test++;
    errno = 0;
    retval = brk((void*)0xC0000000);
    if (retval == -1 && errno == ENOMEM) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psSetEVar() growth: ");
    test++;
    psSetEVar("SAFETEST_A", "alpha");
    psSetEVar("SAFETEST_B", "beta");
    char evalbuf[16];
    memset(evalbuf, 0, sizeof(evalbuf));
    retval = psGetEVar("SAFETEST_B", evalbuf, sizeof(evalbuf));
    if (retval == 0 && !strcmp(evalbuf, "beta")) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psGetEVar() truncation: ");
    test++;
    psSetEVar("SAFETEST_LONG", "abcdef");
    unsigned char evalbuf2[8];
    memset(evalbuf2, 0xAA, sizeof(evalbuf2));
    retval = psGetEVar("SAFETEST_LONG", (char*)evalbuf2, 4);
    int evalbuf2_intact = (retval == 0) && (evalbuf2[0] == 'a') &&
                          (evalbuf2[1] == 'b') && (evalbuf2[2] == 'c') &&
                          (evalbuf2[3] == '\0');
    for (size_t i = 4; i < sizeof(evalbuf2); i++) {
        if (evalbuf2[i] != 0xAA) { evalbuf2_intact = 0; }
    }
    if (evalbuf2_intact) {
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psGetEVar() kernel buffer: ");
    test++;
    retval = psGetEVar("SAFETEST_B", (char*)0xC0000000, 16);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psInterfaceSend() oversized: ");
    test++;
    psInterfaceCreate("safetest:iface", 0);
    retval = psInterfaceSend("safetest:iface", 0, evalbuf, 2 * 1024 * 1024);
    if (retval == -EMSGSIZE) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psInterfaceSend() kernel buffer: ");
    test++;
    retval = psInterfaceSend("safetest:iface", 0, (void*)0xC0000000, 16);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psInterfaceRecieve() kernel buffer: ");
    test++;
    char msgsrc[8] = "message";
    psInterfaceSend("safetest:iface", 0, msgsrc, sizeof(msgsrc));
    size_t msgsize = 0;
    retval = psInterfaceRecieveNB("safetest:iface", (void*)0xC0000000, 16, &msgsize);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("psInterfaceRecieve() valid: ");
    test++;
    char msgdst[8];
    memset(msgdst, 0, sizeof(msgdst));
    retval = psInterfaceRecieveNB("safetest:iface", msgdst, sizeof(msgdst), &msgsize);
    if (retval >= 0 && msgsize == sizeof(msgsrc) && !memcmp(msgdst, msgsrc, sizeof(msgsrc))) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }
    psInterfaceDelete("safetest:iface");

    printf("open() overlong path: ");
    test++;
    char *longpath = malloc(8192);
    memset(longpath, 'a', 8192);
    longpath[8191] = '\0';
    errno = 0;
    open(longpath, 0);
    if (errno == ENAMETOOLONG) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (errno %i)\n", errno);
        fail++;
    }

    printf("exec() overlong path: ");
    test++;
    char *nullargs[] = { NULL };
    retval = psExecA(longpath, nullargs);
    if (retval == -ENAMETOOLONG) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("chdir() overlong path: ");
    test++;
    retval = chdir(longpath);
    free(longpath);
    if (retval == -ENAMETOOLONG) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("exec() invalid path: ");
    test++;
    retval = psExecA(ptr, nullargs);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("exec() invalid argv: ");
    test++;
    retval = psExecA("local:nonexistent", (char**)0xC0000000);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("exec() kernel argv string: ");
    test++;
    char *kernelargs[] = { (char*)0xC0000000, NULL };
    retval = psExecA("local:nonexistent", kernelargs);
    if (retval == -EFAULT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("exec() overlong argv: ");
    test++;
    char *longarg = malloc(70000);
    memset(longarg, 'a', 69999);
    longarg[69999] = '\0';
    char *longargs[] = { longarg, NULL };
    retval = psExecA("local:nonexistent", longargs);
    free(longarg);
    if (retval == -E2BIG) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("exec() missing file: ");
    test++;
    char *missingargs[] = { "exec", NULL };
    retval = psExecA("local:nonexistent", missingargs);
    if (retval == -ENOENT) { 
        printf("success\n"); 
        success++;
    } else {
        printf("fail (%i)\n", retval);
        fail++;
    }

    printf("%i/%i success\n", success, test);
    return 0;
}
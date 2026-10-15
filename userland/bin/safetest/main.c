#include <ps.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sysbase.h>
#include <fcntl.h>

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

    printf("%i/%i success\n", success, test);
    return 0;
}
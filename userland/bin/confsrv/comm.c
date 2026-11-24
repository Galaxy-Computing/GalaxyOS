#include <gcf.h>
#include <errno.h>
#include <ps.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "comm.h"

#define MAX_PACKET_SIZE 4096

unsigned char getbuf[MAX_PACKET_SIZE];
unsigned char getretbuf[MAX_PACKET_SIZE];
unsigned char setterbuf[MAX_PACKET_SIZE];

int lock = 0;

void getthread(void) {
    for (;;) {
        size_t sizeread = 0;
        int sender = psInterfaceRecieve("sys:conf.get", getbuf, MAX_PACKET_SIZE, &sizeread);
        if (sender < 0) continue;

        while (lock) { psYield(); } 
        lock = 1;
        struct confsrv_get_set_param* params = (struct confsrv_get_set_param*)getbuf;

        int ret = 0;

        size_t index = gcf_find_entry(gcfdata, sizegcf, params->name, strlen(params->name));
        if (index == SIZE_MAX) {
            ret = -ENOENT;
            psInterfaceSend("sys:conf.get", sender, &ret, sizeof(int));
        }

        const void *value = gcf_get_entry_data(gcfdata, sizegcf, index);
        int datasize = gcf_get_entry_data_length(gcfdata, sizegcf, index);
        datasize = (MAX_PACKET_SIZE < datasize) ? MAX_PACKET_SIZE : datasize;
        datasize = (params->size < datasize) ? params->size : datasize;

        psInterfaceSend("sys:conf.get", sender, &ret, sizeof(int));
        psInterfaceSend("sys:conf.get", sender, value, datasize);
        lock = 0;
    }
}

void setthread(void) {
    for (;;) {
        size_t sizeread = 0;
        int sender = psInterfaceRecieve("sys:conf.set", setterbuf, MAX_PACKET_SIZE, &sizeread);
        if (sender < 0) continue;

        while (lock) { psYield(); } 
        lock = 1;
        //struct confsrv_get_set_param* params = (struct confsrv_get_set_param*)setterbuf;

        printf("confsrv: reached unimpl rpc call (gcf setter)");
        int ret = -ENOSYS;
        psInterfaceSend("sys:conf.set", sender, &ret, sizeof(int));
        lock = 0;
    }
}
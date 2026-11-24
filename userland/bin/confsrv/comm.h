#ifndef _COMM_H
#define _COMM_H

void getthread(void);
void setthread(void);

extern void *gcfdata;
extern size_t sizegcf;

struct confsrv_get_set_param {
    size_t size;
    char name[];
} __attribute__((packed));

#endif
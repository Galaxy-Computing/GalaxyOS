#ifndef _CONFAPI
#define _CONFAPI

struct confsrv_get_set_param {
    size_t size;
    char name[];
} __attribute__((packed));

int confapiGetValue(char* name, void* buf, size_t size);
int confapiSetValue(char* name, const void* buf, size_t size);

#endif
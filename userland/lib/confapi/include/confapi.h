#ifndef _CONFAPI
#define _CONFAPI

int confapiGetValue(char* name, void* buf, size_t size);
int confapiSetValue(char* name, const void* buf, size_t size);

#endif
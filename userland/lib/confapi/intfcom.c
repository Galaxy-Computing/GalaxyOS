#include <ps.h>
#include <string.h>
#include <stdlib.h>
#include <confapi.h>
#include <stdio.h>

int confapiSetValue(char* name, const void* buf, size_t size) {
    size_t name_len = strlen(name) + 1;
    struct confsrv_get_set_param* params = malloc(sizeof(size_t) + name_len + size);
    params->size = name_len;
    strncpy(params->name, name, name_len);

    memcpy(&(((unsigned char*)params)[sizeof(size_t) + name_len]), buf, size); // this is diabolical

    // "params" here also has the data after it now
    int rval = psInterfaceSend("sys:conf.set", 0, params, sizeof(size_t) + name_len + size);
    if (rval < 0) return rval;

    int comret = 0;
    size_t sizeread = 0;
    rval = psInterfaceRecieve("sys:conf.set", &comret, 1, &sizeread);
    if (rval < 0) return rval;

    return comret;
}

int confapiGetValue(char* name, void* buf, size_t size) {
    size_t name_len = strlen(name) + 1;
    struct confsrv_get_set_param* params = malloc(sizeof(size_t) + name_len);
    params->size = size;
    strncpy(params->name, name, name_len);

    int rval = psInterfaceSend("sys:conf.get", 0, params, sizeof(size_t) + name_len);
    if (rval < 0) return rval;

    // the config server will send us two packets, first being return value and second being data

    int comret = 0;
    size_t sizeread = 0;
    rval = psInterfaceRecieve("sys:conf.get", &comret, 1, &sizeread);
    if (rval < 0) return rval;
    if (comret < 0) return comret;

    // just throw the returned data into the buffer
    rval = psInterfaceRecieve("sys:conf.get", buf, size, &sizeread);
    if (rval < 0) return rval;

    return (int)sizeread;
}
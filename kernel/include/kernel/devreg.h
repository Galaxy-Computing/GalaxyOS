#ifndef _KERNEL_DEVREG_H
#define _KERNEL_DEVREG_H

#include <stdint.h>

#define DEVICE_UNKOWN 0
#define DEVICE_INTERNAL 1
#define DEVICE_HID 2
#define DEVICE_FILESYSTEM 3
#define DEVICE_BLOCK 4
#define DEVICE_DISPLAY 5
#define DEVICE_NETWORK 6

struct device {
    uint32_t id;
    uint8_t k_id;
    uint8_t privilege;
    uint32_t pid; // this doesn't matter for kernel devices
    uint8_t type;
    char* name;
};

uint32_t devreg_register_device(struct device *newdevice);
struct device *devreg_get_device_info(uint32_t devid);
struct device *devreg_find_device_by_kid(uint8_t kid);
void devreg_init(void);

#endif
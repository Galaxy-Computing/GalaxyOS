#ifndef _KERNEL_ATAPI_H
#define _KERNEL_ATAPI_H

#include <kernel/ata.h>

extern int atapi_disc;
extern int atapi_disccount;

int read_cdrom(struct ata_device *device, uint32_t lba, uint32_t sectors, uint16_t *buffer);
void atapi_init(void);

#endif
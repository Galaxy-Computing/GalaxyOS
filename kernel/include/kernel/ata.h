#ifndef _KERNEL_STORAGE_ATA_H
#define _KERNEL_STORAGE_ATA_H

struct ata_device {
    int type; // 0 = no device, 1 = ata, 2 = atapi, 3 = sata, 4 = satapi
    unsigned short bus;
    char select;
    char identifydata[256];
    int identified; // 0 = no
};

extern struct ata_device atadevices[4];

void atapio_detect_disks(void);

#endif
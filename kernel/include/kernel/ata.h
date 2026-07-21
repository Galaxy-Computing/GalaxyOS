#ifndef _KERNEL_ATA_H
#define _KERNEL_ATA_H

#define ATA_TYPE_NONE   0
#define ATA_TYPE_ATA    1
#define ATA_TYPE_ATAPI  2
#define ATA_TYPE_SATA   3
#define ATA_TYPE_SATAPI 4

#define DATA 0
#define ERROR_R 1
#define SECTOR_COUNT 2
#define LBA_LOW 3
#define LBA_MID 4
#define LBA_HIGH 5
#define DRIVE_SELECT 6
#define COMMAND_REGISTER 7

#define CONTROL 0x206

#define ALTERNATE_STATUS 0

struct ata_device {
    int type; // 0 = no device, 1 = ata, 2 = atapi, 3 = sata, 4 = satapi
    unsigned short bus;
    char select;
    char identifydata[256];
    int identified; // 0 = no
};

extern struct ata_device atadevices[4];

void atapio_detect_disks(void);
void ata_io_wait(const uint8_t p);

#endif
/**
 * This file contains mostly generic helper functions used for ATA Disk 
 * initializion & management, but no eg. disk reads or writes here.
 *
 * Be aware. that these functions require you to have IO permissions set
 * so that you can talk to ATA devices.
 *
 * License: MIT
 * Author: k4m1  <k4m1\protonmail.com>
 */

#include <sys/syscalls.h>
#include <sys/io.h>

#include <stdio.h>
#include <stdint.h>

#include "ata.h"

/**
 * ATA spec suggests to check bits 7 and 3 of status register before sending
 * command. However, as _everything_ is slow 'n all, we're basicly having to
 * poll the pin 3 times (aka have enough delay.). With the delay, there's 
 * time for drive to push the correct voltages onto the bus.
 *
 * \param port short Is a IO port to ATA Port base + 7
 * \return The byte read from ATA device.
 */
unsigned char ata_delay_in(short port) {
    unsigned char ret;
    int i;

    for (i = 0; i < 3; i++) {
        ret = inb(port);
    }
    return ret;
}

/**
 * Check if the disk detected is sata or atapi.
 *
 * \param port short Is IO Base to ata device to check
 * \return int disk type, 3 for sata or 4 for atapi. -1 on error.
 */
int ata_check_if_sata_or_atapi(short port) {
    unsigned short disk_id;

    disk_id = (inb(__ATA_CYL_LO(port)) << 8);
    disk_id |= inb(__ATA_CYL_HI(port));

    if (disk_id == 0x3c3c) {
        return 3;
    } else if (disk_id == 0x14EB) {
        return 4;
    } else {
        return -1;
    }
}

/**
 * Check if given ATA disk exists or not
 *
 * \param port short Is IO Base to ata device to check
 * \param type char is primary/secondary(0xA0/0xB0) ID for disk
 * \return int disk type, 0 for no disk, 1 for non-ata, 2 for ata, 
 * 3 for sata, 4 for atapi, and -1 for error.
 */
int ata_check_disk_by_identity(short port, char type) {
    int counter;
    unsigned char iobyte;

    /* First, check that bus isn't floating */
    if (inb(__ATA_STAT(port)) == 0xFF) {
        return -1;
    }

    /* Select drive */
    outb(__ATA_DRIVE_HEAD(port), type);

    /* Set sector count & LBA to 0, ports base + 6,5,4,3 */
    for (counter = 4; counter > 0; counter--) {
        outb(port + 2 + counter, 0);
    }

    /* Send IDENTIFY command */
    outb(__ATA_STAT(port), 0xEC);
    iobyte = ata_delay_in(__ATA_STAT(port));

    /* If status is 0, disk does not exist */
    if (iobyte == 0) {
        return 0;
    }

    /* Check for error indicator in status register */
    iobyte = ata_delay_in(__ATA_STAT(port));

    /* If there is error, then we have sata or atapi disk (most likely) */
    if ((iobyte & __ATA_STAT_ERR) != 0) {
        return ata_check_if_sata_or_atapi(port);
    }

    /**
     * It was not sata or atapi disk, keep polling until status bit 7
     * (__ATA_STAT_WAIT) is clear or timeout is reached.
     *
     */ 
    for (counter = 0; counter < 2000; counter++) {
        if ((inb(__ATA_STAT(port)) & __ATA_STAT_WAIT) == 0) {
            break;
        }
    }
    /* Check for timeout */
    if (counter == 2000) {
        return -1;
    }

    /**
     * Read LBA low & mid values, if they're not set to zero, this is
     * non-ata disk
     */
    if (inb(__ATA_CYL_LO(port)) != 0) {
        return 1;
    }
    if (inb(__ATA_CYL_HI(port)) != 0) {
        return 1;
    }

    return 2;
}

/**
 * Helper function to show users pretty messages. 
 *
 * \param type Int what kind of disk was detected
 * \return pointer to disk id string related to disk id
 */
char *
ata_get_disk_str_by_id(int id)
{
    switch (id) {
        case(__ATA_DISK_ID_INTERNAL_ATAPI):
            return __ATA_DISK_ID_STR_ATAPI;
        case(__ATA_DISK_ID_INTERNAL_SATA):
            return __ATA_DISK_ID_STR_SATA;
        case(__ATA_DISK_ID_INTERNAL_ATA):
            return __ATA_DISK_ID_STR_ATA;
        default:
            return __ATA_DISK_ID_STR_NON_ATA;
    }
}


/* TODO: doc */
int detect_ata_disks(void) {
    uint64_t stat = 0;
    int ports[] = {0x1f0, 0x170, 0x1e8, 0x168};
    int ret;
    int i;

    for (i = 0; i < 4; i++) {
        sc_do_hardware_ioperm(ports[i], 10, 1, &stat);
        ret = ata_check_disk_by_identity(ports[i], 0xA0);

        if (ret == -1) {
            printf("Disk detect for 0x%x failed, bus floats\n", ports[i]);
        } else {
            printf("Found %s disk.\n", ata_get_disk_str_by_id(ret));
        }
    }
}

int main() {
    detect_ata_disks();

    do {} while (1);
}





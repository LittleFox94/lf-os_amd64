/*
 * Simple ATAPI driver to provide disk read/write ops
 *
 * License: MIT
 * Author: k4m1  <k4m1@protonmail.com>
 *
 */

#include <sys/types.h>
#include <sys/io.h>
#include <sys/syscalls.h>

#include <stdio.h>
#include <stdint.h>

#include "ata.h"
#include "scsi.h"
#include "atapi.h"


/**
 * Read sectors from atapi disk
 *
 * \param unsigned int port Is port to read from
 * \param unsigned int drive Is the disk we want to read
 * \param lba *lbaptr Is pointer to LBA structure
 * \param unsigned short *dst Is pointer to destination buffer for read
 * \param char sectors Is amount of sectors you want to read
 * \return 0 on success or Non-zero error code on error.
 */
int atapi_read_sector_pio(unsigned int port, unsigned int drive, 
        struct lba *lbaptr, unsigned char *dst, char sectors) {
    /* We use 12 bytes long read command buffer */
    unsigned char cmd[12] = {0};
    unsigned char stat;
    int sector_size;
    char offset;

    /* Select drive, and set other settings */
    outb(__ATA_DRIVE_HEAD(port), drive & (1 << 4));
    ata_delay_in(port);

    /** 
     * Set PIO Mode, (DMA will arrive later), as well as 
     * sector size
     */
    outb(__ATA_ERR(port), 0x00);
    outb(__ATA_CYL_LO(port), 0x00);
    outb(__ATA_CYL_HI(port), 0x10);

    /* Send ATA PACKET */
    outb(__ATA_STAT(port), 0xA0);

    printf("ATA PACKET sent, polling for bsy...\n");
    /* Poll for bsy */
    do { } while ((inb(__ATA_STAT(port)) & __ATA_STAT_WAIT) != 0);
    asm volatile("pause");
    printf("BSY done, polling for RDY...\n");
    do { } while ((inb(__ATA_STAT(port)) & __ATA_STAT_RDY) == 1);
    asm volatile("pause");

    printf("Got RDY, checking for errors...");
    if (stat & __ATA_STAT_ERR) {
        stat = inb(__ATA_ERR(port));
        if (stat & __ATA_STAT_ERR) {
            printf("\nError set: %x\n", stat);
            return __ATA_CMD_READ_ERR;
        }
    }
    printf(" Ok\n");

    printf("Writing commad buffer to ATA DATA\n");
    cmd[9] = 1;
    cmd[2] = lbaptr->low;
    cmd[3] = lbaptr->mid;
    cmd[4] = lbaptr->high;
    cmd[5] = lbaptr->even_higher;

    for (int i = 0; i < 12; i++) {
        outb(__ATA_DATA(port), cmd[i]);
    }

    printf("Polling for BSY & RDY...\n");
    do { } while ((inb(__ATA_STAT(port)) & __ATA_STAT_WAIT) == 1);
    stat = inb(__ATA_STAT(port));
    if ((stat & __ATA_STAT_ERR) || !(stat & __ATA_STAT_RDY)) {
        stat = inb(__ATA_ERR(port));
        if (!stat) {
            printf("Got error, but now cleared.\n");
        } else {
            printf("Error set: %x\n", stat);
            return __ATA_CMD_READ_ERR;
        }
    }
    
    printf("Got BSY & RDY, getting sector size...\n");
    sector_size  = inb(__ATA_CYL_HI(port));
    sector_size  = sector_size << 8;
    sector_size |= inb(__ATA_CYL_LO(port));

    printf("atapi sector size: %d\n", sector_size);
    printf("reading data to: %p... ", dst);

    uint64_t err;

    for (int z = 0; z < 128; z++) dst[z] = 'A';
    sc_do_hardware_ata_read_cli(
            port,
            sector_size,
            dst,
            &err
    );

    printf("status: %d\n", err);
    /*
    printf("Waiting for disk to finish...\n");
    do {
        stat = inb(__ATA_STAT(port));
        stat &= (__ATA_STAT_WAIT | __ATA_STAT_RDY);
    } while (stat);
    */

    printf("First 8 bytes of data:\n");
    for (int off = 0; off < 8; off++) {
        printf("%x ", dst[off]);
    }
    printf("\n");
    return 0;
}


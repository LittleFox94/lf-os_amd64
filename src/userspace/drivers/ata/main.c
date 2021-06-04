/**
 * Define some constants that are shared across different drivers talking ATA
 *
 * LICense: MIT
 * Author: k4m1  <k4m1@protonmail.com>
 */

#include <sys/syscalls.h>
#include <sys/io.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ata.h"

#define __VALID_DISK(x) (((x & __ATA_DISK_ID_INTERNAL_NOT_DISK) == 0) && \
                         ((x & __ATA_DISK_ID_INTERNAL_NON_ATA) == 0))

/**
 * Implement manual cache flush - as some drives don't flush after ops.
 *
 * \param bus short Is IO-base to bus to operate with.
 * \return 0 on success or -1 on error
 */
int ata_cache_flush(short bus) {
    unsigned char stat;

    outb(__ATA_STAT(bus), __ATA_CMD_CFLUSH);

    /* We don't have timers yet, use for loop for polling. */
    /* TODO: Once we have timers, use them */
    for (int i = 0; i < 50; i++) {
        stat = inb(__ATA_STAT(bus));
        if ((stat & __ATA_STAT_WAIT) == 0) {
            return 0;
        }
    }
    return -1;
}

/**
 * Do software reset, handy if disk is malfunctioning.
 *
 * \param bus short Is IO-base to bus to operate with
 */
void ata_sw_reset(short bus) {
    unsigned char stat;

    /**
     * Get original status, set reset bit, write out, wait, clear reset,
     * write original status back.
     */
    stat = inb(__ATA_STAT(bus));
    stat |= 4;
    outb(__ATA_STAT(bus), stat);
    asm volatile("nop");
    asm volatile("nop");
    stat ^= (1 << 3);
    outb(__ATA_STAT(bus), stat);

}
    
/**
 * Check for a floating bus. Use status reigster as a way to check this,
 * as bits 1 & 2 should always be zero. If bus floats, we'll only read 1's
 * back in (as bus is 'pulled up' to constant 5v
 *
 * \param bus short is IO-base to bus to operate with
 * \returns 0 if bus does _NOT_ float or 1 if bus floats.
 */
int ata_check_fbus(short bus) {
    unsigned char stat;
    
    ATA_DELAY_IN(stat, bus);
    if (stat == 0xFF) {
        return 1;
    }
    return 0;
}

/**
 * Non standard ways of detecting disks, don't use these if possible.
 * The methods have been tested & seem to wrok w/ qemu, but 
 * no experiments have been conducted with real hardware with these
 * functions.
 *
 * First detection method is to select it, and check if bit 7 in status
 * register is set. If it is, we have a drive.
 *
 * This wont work with atapi devices, as they've got bit 7 clear until first
 * command packet is sent to them.
 *
 * \param bus short Is IO-base to bus to operate with
 * \param disk char Is which disk to check (0 / 1)
 * \return 0 if disk doesn't exist, or 1 if it does.
 */
int ata_nonstd_detect_disk_by_select(short bus, char disk) {
    unsigned char stat;

    stat = inb(__ATA_DRIVE_HEAD(bus));
     
    /* Toggle drive bit with disk value */
    disk = disk << 4;
    stat ^= disk;

    /* Select the disk */
    outb(__ATA_DRIVE_HEAD(bus), stat);
    ATA_DELAY_IN(stat, bus);

    if ((stat & __ATA_STAT_CDE) == 0) {
        return 0;
    }
    return 1;
}

/**
 * Detect disk in non std way, attempt 2. Detect by running Device Diagnostics
 * and checking error register content. If disk exists, error register should
 * be set to all 1's
 *
 * \param bus short Is the IO-base to bus.
 * \return 0 if disk does not exist, 1 if it does.
 */
int ata_nonstd_detect_disk_by_dd(short bus) {
    unsigned char stat;

    outb(__ATA_STAT(bus), __ATA_CMD_DD);
    stat = inb(__ATA_ERR(bus));

    if (stat != 0xFF) {
        return 0;
    }
    return 1;
}

/**
 * "Function" used to read disk info after disk detection by identify 
 * command. 
 * NOTE:
 *  At least on qemu, you _must_ read disk info before performing any other
 *  actions, disk will ignore disk read etc. commands othervice & just wait
 *  until you start _any_ read from bus io-base, and then yeets the disk 
 *  info at you.. Totally regardless of anything else (apart from reset) done
 *  to the disk....
 *
 *
 *  Oh well, anyway
 *  
 * \param bus short Is the IO-base to bus to read disk info from
 * \param dst short * Is pointer where to read the disk info
 */
static inline void read_disk_info(short bus, short *dst);
static inline void read_disk_info(short bus, short *dst) {
    for (int __dst_off = 0; __dst_off < 256; __dst_off++) { 
        dst[__dst_off] = inw(bus); 
    } 
}

/**
 * Detect ata disk with Identify command, de-facto way of detecting disks.
 *
 * \param bus short Is the IO-base to bus
 * \param disk char Is primary/secondary disk identifier (0xA0/0xB0)
 * \return according to ATA_DISK_TYPE_<type> or 0xFF if bus floats.
 */
int ata_detect_disk_by_identify(short bus, char type) {
    int counter;
    uint16_t id;
    unsigned char stat;

    /* Start by checking bus (float or nah */
    if (ata_check_fbus(bus)) {
        return 0xFF;
    }

    /* Select drive */
    outb(__ATA_DRIVE_HEAD(bus), type);

    /* Set sector count and LBA to 0 */
    for (int i = 2; i < 5; i++) {
        outb((bus + i), 0);
    }

    /* Send identify */
    outb(__ATA_STAT(bus), __ATA_CMD_IDENTIFY);
    ATA_DELAY_IN(stat, bus);

    /**
     * Check status, if error bit is set, this is sata or atapi device
     *
     */
    if ((stat & __ATA_STAT_ERR) != 0) {
        id = inb(__ATA_CYL_LO(bus)) << 8;
        id |= (uint16_t)(inb(__ATA_CYL_HI(bus)));
        if (id == __ATA_DISK_ID_SATA) {
            return __ATA_DISK_ID_INTERNAL_SATA;
        }
        if (id == __ATA_DISK_ID_ATAPI) {
            return __ATA_DISK_ID_INTERNAL_ATAPI;
        }
    }

    /* Keep polling until bit 7 is set, or timeout */
    /* TODO: Once we have timers, use them */
    for (counter = 0; counter < 5000; counter++) {
        if ((stat & __ATA_STAT_WAIT) == 0) {
            break;
        }
        ATA_DELAY_IN(stat, bus);
    }
    if (counter == 5000) {
        /* Timeout */
        return __ATA_DISK_ID_INTERNAL_NOT_DISK;
    }
    if (inb(__ATA_SECTOR_NUM(bus)) || inb(__ATA_CYL_LO(bus))) {
        return __ATA_DISK_ID_INTERNAL_NON_ATA;
    }

    /* Yet another poll to verify this is ata disk :/ */
    /* TODO: Once we have timers, use them */
    for (counter = 0; counter < 5000; counter++) {
        ATA_DELAY_IN(stat, bus);
        if ((stat & __ATA_STAT_RDY) != 0) {
            break;
        }
        if ((stat & __ATA_STAT_ERR) != 0) {
            break;
        }
    }
    if (counter == 5000) {
        /* Timeout */
        return __ATA_DISK_ID_INTERNAL_NOT_DISK;
    }

    return __ATA_DISK_ID_INTERNAL_ATA;
}

void print_disk_type(short bus, int slot, int type) {
    printf("Bus: 0x%x, slot: ", bus);
    if (slot == 0xA0) {
        printf("primary");
    } else {
        printf("secondary");
    }
    printf(", type: ");

    switch (type) {
        case (__ATA_DISK_ID_INTERNAL_NOT_DISK):
            printf("No disk attached\n");
            break;
        case (__ATA_DISK_ID_INTERNAL_NON_ATA):
            printf("%s\n", __ATA_DISK_ID_STR_NON_ATA);
            break;
        case (__ATA_DISK_ID_INTERNAL_ATA):
            printf("%s\n", __ATA_DISK_ID_STR_ATA);
            break;
        case (__ATA_DISK_ID_INTERNAL_ATAPI):
            printf("%s\n", __ATA_DISK_ID_STR_ATAPI);
            break;
        case (__ATA_DISK_ID_INTERNAL_SATA):
            printf("%s\n", __ATA_DISK_ID_STR_SATA);
            break;
        default:
            printf("BUG, disk id out of range!\n");
    }
}

/**
 * Read ata disk in 28-bit pio mode.
 *
 * \param bus short Is bus to read from
 * \param disk int Is disk to read from, NOTE: 0xE0 for pri, 0xF0 for sec
 * \param cnt char Is sector count to read 
 * \param dst void* Is destination buffer to read to
 * \param lba uint32_t Is LBA
 * \return 0 on success or error code as defined in ata.h
 */
int read_ata_disk_b28(short bus, 
        int disk,
        char cnt, 
        void *dst, 
        uint32_t lba) {

    /* retry attempts */
    int retries_done = 0;

    uint16_t *dstptr = (uint16_t*)dst;
    unsigned char stat;

start:
    /* Check if bus floats or not */
    if (ata_check_fbus(bus)) {
        if (retries_done) {
            return __ATA_CMD_BUS_FLOATS;
        }
        ata_sw_reset(bus);
        retries_done++;
    }

    /* Check disk status */
    if (inb(__ATA_STAT(bus)) & __ATA_STAT_WAIT) {
        /* Stat wait set, reset disk */
        if (retries_done) {
            return __ATA_CMD_TIMEOUT;
        }
        retries_done++;
        goto start;
    }

    /* set sector count */
    outb(__ATA_SECTOR_CNT(bus), cnt);
    
    /* set LBA */
    ATA_SET_LBA(bus, lba);

    /* LBA bits 24 - 28 OR'ed with device pri/sec flag */
    stat = (unsigned char)((lba >> 24) & 0x0000000F);
    stat |= disk;
    outb(__ATA_DRIVE_HEAD(bus), stat);

    /* Send read command, then wait until disk is ready */
    outb(__ATA_STAT(bus), __ATA_CMD_READ);

    /* TODO: Once we have timers, use them */
    for (int i = 0; i < 5000; i++) {
        ATA_DELAY_IN(stat, bus);
        if ((stat & __ATA_STAT_WAIT) == 0) {
            break;
        }
    }
    if ((stat & __ATA_STAT_WAIT) != 0) {
        return __ATA_CMD_TIMEOUT;
    }

    /* Now read sectors */
    for (int offset = 0; offset < (256 * cnt); offset++) {
        dstptr[offset] = inw(bus);
        if ((offset != 0) && ((offset % 256) == 0)) {
            /* Check status register */
            stat = inb(__ATA_STAT(bus));
            if ((stat & __ATA_STAT_ERR) != 0) {
                return __ATA_CMD_READ_ERROR;
            }
        }
    }

    /* Final check for read errors */
    stat = inb(__ATA_STAT(bus));
    if ((stat & __ATA_STAT_ERR) != 0) {
        return __ATA_CMD_READ_ERROR;
    }
    return __ATA_CMD_SUCCESS;
}

/**
 * Helper function to populate ata_disk_stat
 *
 * This function is not intended to be called outside this file,
 * so this documentation shall be enough.
 */
static int ata_populate_ata_disk_stat(struct ata_disk_stat ata_stat, short bus, int stat) {
    ata_stat.bus       = bus;
    ata_stat.id        = stat;
    ata_stat.disk_info = (short *)malloc(512);
    if (ata_stat.disk_info == 0) {
       return -1;
    }
    read_disk_info(bus, ata_stat.disk_info);
    return 0;
}

/**
 * This function is used to detect all disks & populate ata disk info
 * structures (refer to ata.h) 
 *
 * \param ata_sp struct ata_disk_stat Is a pointer to ata_disk_stat structure
 * array to use, there needs to be 8 entries in this list.
 */
void ata_detect_disks(struct ata_disk_stat *ata_sp) {
    short bus_list[4] = {0x1f0, 0x170, 0x1e8, 0x168};
    uint64_t err = 0;
    int stat = 0;
    int ata_sp_offset = 0;

    for (int i = 0; i < 4; i++) {
        /* TODO: Check for errors */
        sc_do_hardware_ioperm(bus_list[i], 8, 1, &err);
        
        /* Check for disk, prints should be removed if not wanted here */
        stat = ata_detect_disk_by_identify(bus_list[i], 0xA0);
        if (stat == 0xFF) {
            /* Disk floats */
            printf("Bus 0x%x floats, skip disk detection\n", bus_list[i]);
            goto revoke_ioperm;
        }
        print_disk_type(bus_list[i], 0xA0, stat);
        if (__VALID_DISK(stat)) {
            if (ata_populate_ata_disk_stat(ata_sp[ata_sp_offset], bus_list[i], stat)) {
                printf("FATAL: failed to allocate space for ata_sp[%d].disk_info!\n",
                        ata_sp_offset);
                return;
            }
        }
        ata_sp_offset++;

        /** 
         * No need to recheck for floating bus here,
         * just print disk type for secondary device too
         */
        stat = ata_detect_disk_by_identify(bus_list[i], 0xB0);
        print_disk_type(bus_list[i], 0xB0, stat);
        if (__VALID_DISK(stat)) {
           if (ata_populate_ata_disk_stat(ata_sp[ata_sp_offset], bus_list[i], stat)) {
               printf("FATAL: failed to allocate space for ata_sp[%d].disk_info!\n",
                      ata_sp_offset);
               return;
           }
        }

revoke_ioperm:
        /* Revoke ioperms for this device & move on to next one */
        sc_do_hardware_ioperm(bus_list[i], 8, 0, &err);
    }
}

int main() {
    struct ata_disk_stat ata_sp[8] = {0};

    ata_detect_disks(ata_sp);

    do { } while (1);
    return 0;
}

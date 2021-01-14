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

#include <sys/io.h>

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
 * Helper to check ata disk status
 *
 * \param port short Is IO port to device status register
 * \return 1 if device is ready, or 0 if busy
 */
int ata_device_ready(short port) {
    int status;

    status = ata_delay_in(port);
    if ((status & __ATA_STAT_RDY) && ((status & __ATA_STAT_WAIT) == 0)) {
        return 1;
    }
    return 0;
}

/**
 * Waiter for ata device to become ready, we will trigger error to indicate
 * user if disk timed out, TODO: figure out ways to print stuff
 *
 * \param port short Is IO port to device status register
 * \return 0 on success or -1 on timeout
 */
int ata_device_wait(short port)
{
    int counter;
    int ret = -1;
    int reset_done = 0;
    
start:
    for (counter = 0xFFFF; counter > 0; counter--) {
        if (ata_device_ready(port)) {
            ret = 0;
            break;
        }
    }
    if (!reset_done) {
        ata_sw_reset(port);
        reset_done = 1;
        goto start;
    }

    return ret;
}

/**
 * Do manual cache flush - as some drives require.
 * In case the drive does not do this, temporary bad sectors may be created,
 * or the subsequent writes might fail invisibly. Be careful with this one!
 *
 * \param port short Is a IO base + offset to command register
 * \return 0 on success or non-zero on error
 */
int ata_cache_flush(short port) {
    int counter;            // Used to poll in until clear flag is set
    int ret = -1;

    if (ata_device_wait(port)) {
        return ret;
    }

    outb(0xE7, port);
    for (counter = 0; counter < 50; counter++) {
        if ((inb(port) & __ATA_STAT_WAIT) == 0) {
            ret = 0;
            break;
        }
    }
    return ret;
}

/**
 * Perform software reset to ATA device, this could prove useful on some
 * errors etc. (Have you tried turning it off and on again)
 *
 * The short NOP delay is to give slower ATA disks time to react.
 *
 * \param port short Is control register of this bus
 * \return Nothing.
 */
void ata_sw_reset(short port) {
    unsigned short cmd_status_byte;

    /* Read in status register, OR it by 4 to set SRST = 4, then write back */
    cmd_status_byte = inb(port);
    cmd_status_byte |= 0x4;
    outb(port, cmd_status_byte);

    /* delay to let reset happen */
    NOP();
    NOP();

    /* Clear SRST bit, and write command back */
    cmd_status_byte &= 0xfb;
    outb(port, cmd_status_byte);
}

/**
 * Check for floating bus by reading status register.
 * As status reg bits 1 and 2 should always be 0, them being 1's tells us
 * that something is wrong. Additionally, if we only read 1's back and end
 * up with status register value FF, we know bus floats.
 *
 * \param port short Is a status register to bus you want to check
 * \return non-zero if bus floats, or 0 if bus does not float.
 */
int ata_check_bus(short port) {
    if (inb(port) == 0xFF) {
        return 1;
    }
    return 0;
}

/**
 * Non-standard way of detecting drives, use of this function should be as
 * last resort when nothing else works.
 *
 * This function wont work for ATAPI disks.
 *
 * \param port short Is port to drive/head register for device to check for
 * \param disk_num unsigned char Is disk number for disk to check (0 or 1)
 * \return -1 on err, other non-zero if disk exists, or 0 if disk does not exist
 */
int ata_nonstd_disk_detect_by_select(short port, unsigned char disk_num)
{
    int ret = 0;
    int iobyte;

    /* Read register, and toggle drive bit */
    iobyte = inb(port);
    disk_num << 4;
    iobyte ^= disk_num;
    outb(iobyte, port);

    /* Read status register */
    port += 1;
    iobyte = ata_delay_in(port);

    /* If __ATA_STAT_CDE is set, then we have a drive. */
    ret = iobyte & __ATA_STAT_CDE;
    return ret;

}

/**
 * Non-standard way of detecting drives, use of this function should be as
 * last resort when nothing else works.
 *
 * Detect drive by running device diagnostics command, and then checking
 * error register. Allegedly DD command on existing device sets bit in the reg
 * but I'm not 100% sure of this.
 *
 * \param port short Is status/command port of your drive
 * \return -1 on error, other non-zero if device is detected, 0 if not.
 */
int ata_nostd_disk_detect_by_dd(short port) {
    if (ata_device_wait(port)) {
        return -1;
    }

    /* Write device diagnostics command */
    outb(0x90, port);

    /* Read error register */
    port -= 6;
    ret = inb(port);

    return (ret == 0xFF);
}

/**
 * Detect ATA disk by Identify command, this seems to be the correct way to
 * do ata disk detection.
 *
 * \param port short Is IO Base for device to detect
 * \param type unsigned char Is primary/secondary device ID (0xA0/0xB0)
 * \return -1 on error,
 *          0 if disk is not detected
 *          1 if disk is detected, but it's not ATA
 *          2 if some form of ATA disk is detected
 *          3 if detected disk is SATA
 *          4 if detected disk is ATAPI
 */
int ata_disk_detect_by_id(short port, unsigned char type) {
    int cnt;
    short disk_id;
    unsigned char io_byte;
    
    if ((type != 0xA0) && (type != 0xB0)) {
        return -1;
    }

    /* Select drive */
    port += 6;
    outb(type, port);

    /* set sector count & LBA values to 0 */
    for (cnt = 0; cnt < 4; cnt++) {
        outb(0, port - cnt);
    }

    /* Wait for ready */
    port++;
    if (ata_device_wait(port)) {
        return -1;
    }

    /* Send identify command */
    outb(0xEC, port);
    io_byte = ata_delay_in(port);
    
    /* If status register == 0, disk doesn't exist */
    if (io_byte == 0) {
        return 0;
    }
    
    /* Wait until disk is ready */
    if (ata_delay_in(port)) {
        return -1;
    }
    io_byte = ata_delay_in(port);

    /* If error bit in status register is set, then disk is sata or atapi */
    if ((io_byte & __ATA_STAT_ERR) == 1) {
        /* Disk is sata or ATAPI */
        port -= 3;
        io_byte = inb(port);
        disk_id = ((short)io_byte << 8);
        io_byte = inb(port--);
        disk_id |= io_byte;

        if (disk_id == 0x3c3c) {
            return 3;
        }
        if (disk_id == 0x14eb) {
            return 4;
        }
    }
    


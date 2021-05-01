/**
 * Define some constants that are shared across different drivers talking ATA
 *
 * LICense: MIT
 * Author: k4m1  <k4m1@protonmail.com>
 */

#include <sys/io.h>
#include <sys/syscalls.h>

#include <stdint.h>

#include "ata.h"

/**
 * Helper functions that are shared across ata driver(s)
 *
 */

/**
 * Poll ata status register few times to get correct data out, polls
 * give the disk to push correct voltages to the bus.
 *
 * \param port short Is a port to IO-base for device to read from
 * \return unsigned char value from status register
 */
unsigned char ata_delay_in(short port) {
    unsigned char ret;
    for (int i = 0; i < 5; i++) {
        ret = inb(__ATA_STAT(port));
    }
    return ret;
}

/**
 *
 * Do manual cache flush, used if drive doesn't do flushing.
 * If drive deosn't do flushes, temp. bad sectors may be created, or 
 * subsequent writes might fail invisibly.
 *
 * \param port short Is a port to IO-base for device to operate with
 * \return 0 on success or -1 on error
 */
void ata_cache_flush(short port) {
    unsigned char stat;
    outb(__ATA_STAT(port), __ATA_CMD_CFLUSH);

    /**
     * Whilst we don't have timers, we'll use for loop for timer,
     * wait polling up to something around 5000ns 
     */
    for (int i = 0; i < 50; i++) {
        stat = inb(__ATA_STAT(port));
        if ((stat & __ATA_STAT_WAIT) == 0) {
            return 0;
        }
    }
    return -1;
}

/**
 * Do software reset for ATA device, useful if device is malfunctioning.
 *
 * \param port short Is port to IO-base of disk
 */
void ata_sw_reset(short port) {
    unsigned char cmd;

    /* Read current status */
    cmd = inb(__ATA_STAT(port));

    /* Set reset bit */
    cmd |= 4;

    /* Write reset */
    outb(__ATA_STAT(port), cmd);

    /* Wait for few ticks until disk has done it's reset */
    asm("nop");
    asm("nop");

    /* Clear reset & write original contents back to cmd/stat */
    cmd &= 0xFB;
    outb(__ATA_STAT(port), cmd);
}






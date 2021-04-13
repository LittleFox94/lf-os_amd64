/**
 * Define some constants that are shared across different drivers talking ATAPI
 *
 * License: MIT
 * Author: k4m1  <k4m1@protonmail.com>
 *
 */

#ifndef __ATAPI_H__
#define __ATAPI_H__

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
        struct lba *lbaptr, unsigned char *dst, char sectors);

#endif /* __ATAPI_H__ */

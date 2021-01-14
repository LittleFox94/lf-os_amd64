/**
 * Define some constants that are shared across different drivers talking ATA
 *
 * LICense: MIT
 * Author: k4m1  <k4m1@protonmail.com>
 */

#ifndef __ATA_H__
#define __ATA_H__

#define __ATA_DR            0x170        // DATA Register
#define __ATA_ERFR          0x171        // Error/Feature Register
#define __ATA_SCR           0x172        // Sector Count Register
#define __ATA_SNR           0x173        // Sector Number Register (LBA low)
#define __ATA_CLR           0x174        // Cylinder low / LMA mid
#define __ATA_CHR           0x175        // cylinder high / LMA high
#define __ATA_DHR           0x176        // drive/head Register
#define __ATA_SRCR          0x177        // Status/Command Register

#define __ATA_ASDCR         0x3F6        // Alternate Status 
                                         // & DevICe Control Register
#define __ATA_DAT           0x3F7        // Drive Address Register

// ATA error bit definitions
#define __ATA_ERR_AMNF      0x01         // Address Mark Not Found
#define __ATA_ERR_TKZNF     0x02         // Track Zero Not Fount
#define __ATA_ERR_ABRD      0x04         // Command Aborted
#define __ATA_ERR_MCR       0x08         // Media Change Request
#define __ATA_IDNF          0x10         // ID not found
#define __ATA_MC            0x20         // Media Changed
#define __ATA_UNC           0x40         // Uncorrectable DATA Error
#define __ATA_BBD           0x80         // Bad Block Detected

// status register bit definitions
#define __ATA_STAT_ERR      0x01         // Error
#define __ATA_STAT_IC       0x06         // Index & correced data, always 0
#define __ATA_STAT_RDY      0x08         // Drive has data/is ready to recv
#define __ATA_STAT_OMSR     0x10         // Overlapped Mode ServICe Request
#define __ATA_STAT_DFE      0x20         // Drive fault ERR (does not set ERR!)
#define __ATA_STAT_CDE      0x40         // Clear on error / drive spun down
#define __ATA_STAT_WAIT     0x80         // Drive is preparing for send/recv

// devICe control register bit definitions
#define __ATA_DCR_ZERO      0x01         // Always zero
#define __ATA_DCR_CLI       0x02         // Set to disable intERRupts
#define __ATA_DCR_RST       0x04         // Set to do reset on bus
#define __ATA_DCR_RSVD      0x78         // Reserved
#define __ATA_DCR_HOB       0x80         // Set this to read the High Order Byte
                                         // of the last LBA48 value sent.

// drive address register bit definitions
#define __ATA_DAR_DS0       0x01         // Drive Select 0
#define __ATA_DAR_DS1       0x02         // Drive Select 1
#define __ATA_DAR_CSH       0x3c         // Currently Selected Head
                                         //   as compliment of one.
#define __ATA_DAR_WG        0x40         // Write Gate (0 when write active)
#define __ATA_DAR_RSVD      0x80         // Reserved

/**
 * Helper macro to allow easy access to NOPs for few hundred-thousand
 * nanosecond delays needed by some ATA related operations.
 *
 * This is defined as volatile & 'memory' so that compilers realize
 * not to touch this seemingly useless bit of code.
 *
 */
#define NOP() (asm volatile("nop":::"memory");


#endif  /* __ATA_H__ */

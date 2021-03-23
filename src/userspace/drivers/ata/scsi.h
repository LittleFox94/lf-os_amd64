/**
 * Define some constants that are shared across different drivers talking SCSI
 *
 * License: MIT
 * Author: k4m1  <k4m1@protonmail.com>
 *
 */

#ifndef __SCSI_H__
#define __SCSI_H__

#define SCSI_TEST_UNIT_READY                 0x00
#define SCSI_REQUEST_SENSE                   0x03
#define SCSI_FORMAT_UNIT                     0x04
#define SCSI_INQUIRY                         0x12
#define SCSI_START_STOP_UNIT                 0x1B
#define SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL    0x1E
#define SCSI_READ_FORMAT_CAPACITIES          0x23
#define SCSI_READ_CAPACITY                   0x25
#define SCSI_READ                            0x28
#define SCSI_WRITE                           0x2A
#define SCSI_SEEK                            0x2B
#define SCSI_WRITE_AND_VERIFY                0x2E
#define SCSI_VERIFY                          0x2F
#define SCSI_SYNCHRONIZE_CACHE               0x35
#define SCSI_WRITE_BUFFER                    0x3B
#define SCSI_READ_BUFFER                     0x3C
#define SCSI_READ_TOC/PMA/ATIP               0x43
#define SCSI_GET_CONFIGURATION               0x46
#define SCSI_GET_EVENT_STATUS_NOTIFICATION   0x4A
#define SCSI_READ_DISC_INFORMATION           0x51
#define SCSI_READ_TRACK_INFORMATION          0x52
#define SCSI_RESERVE_TRACK                   0x53
#define SCSI_SEND_OPC_INFORMATION            0x54
#define SCSI_MODE_SELECT                     0x55
#define SCSI_REPAIR_TRACK                    0x58
#define SCSI_MODE_SENSE                      0x5A
#define SCSI_CLOSE_TRACK_SESSION             0x5B
#define SCSI_READ_BUFFER_CAPACITY            0x5C
#define SCSI_SEND_CUE_SHEET                  0x5D
#define SCSI_REPORT_LUNS                     0xA0
#define SCSI_BLANK                           0xA1
#define SCSI_SECURITY_PROTOCOL_IN            0xA2
#define SCSI_SEND_KEY                        0xA3
#define SCSI_REPORT_KEY                      0xA4
#define SCSI_LOAD/UNLOAD_MEDIUM              0xA6
#define SCSI_SET_READ_AHEAD                  0xA7
#define SCSI_READ                            0xA8
#define SCSI_WRITE                           0xAA
#define SCSI_READ_MEDIA_SERIAL_NUMBER        0xAB
#define SCSI_SERVICE_ACTION_IN               0x01
#define SCSI_GET_PERFORMANCE                 0xAC
#define SCSI_READ_DISC_STRUCTURE             0xAD
#define SCSI_SECURITY_PROTOCOL_OUT           0xB5
#define SCSI_SET_STREAMING                   0xB6
#define SCSI_READ_CD_MSF                     0xB9
#define SCSI_SET_CD_SPEED                    0xBB
#define SCSI_MECHANISM_STATUS                0xBD
#define SCSI_READ_CD                         0xBE
#define SCSI_SEND_DISC_STRUCTURE             0xBF

#endif /* __SCSI_H__ */

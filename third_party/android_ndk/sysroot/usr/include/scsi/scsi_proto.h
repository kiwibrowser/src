/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _SCSI_PROTO_H_
#define _SCSI_PROTO_H_
#include <linux/types.h>
#define TEST_UNIT_READY 0x00
#define REZERO_UNIT 0x01
#define REQUEST_SENSE 0x03
#define FORMAT_UNIT 0x04
#define READ_BLOCK_LIMITS 0x05
#define REASSIGN_BLOCKS 0x07
#define INITIALIZE_ELEMENT_STATUS 0x07
#define READ_6 0x08
#define WRITE_6 0x0a
#define SEEK_6 0x0b
#define READ_REVERSE 0x0f
#define WRITE_FILEMARKS 0x10
#define SPACE 0x11
#define INQUIRY 0x12
#define RECOVER_BUFFERED_DATA 0x14
#define MODE_SELECT 0x15
#define RESERVE 0x16
#define RELEASE 0x17
#define COPY 0x18
#define ERASE 0x19
#define MODE_SENSE 0x1a
#define START_STOP 0x1b
#define RECEIVE_DIAGNOSTIC 0x1c
#define SEND_DIAGNOSTIC 0x1d
#define ALLOW_MEDIUM_REMOVAL 0x1e
#define READ_FORMAT_CAPACITIES 0x23
#define SET_WINDOW 0x24
#define READ_CAPACITY 0x25
#define READ_10 0x28
#define WRITE_10 0x2a
#define SEEK_10 0x2b
#define POSITION_TO_ELEMENT 0x2b
#define WRITE_VERIFY 0x2e
#define VERIFY 0x2f
#define SEARCH_HIGH 0x30
#define SEARCH_EQUAL 0x31
#define SEARCH_LOW 0x32
#define SET_LIMITS 0x33
#define PRE_FETCH 0x34
#define READ_POSITION 0x34
#define SYNCHRONIZE_CACHE 0x35
#define LOCK_UNLOCK_CACHE 0x36
#define READ_DEFECT_DATA 0x37
#define MEDIUM_SCAN 0x38
#define COMPARE 0x39
#define COPY_VERIFY 0x3a
#define WRITE_BUFFER 0x3b
#define READ_BUFFER 0x3c
#define UPDATE_BLOCK 0x3d
#define READ_LONG 0x3e
#define WRITE_LONG 0x3f
#define CHANGE_DEFINITION 0x40
#define WRITE_SAME 0x41
#define UNMAP 0x42
#define READ_TOC 0x43
#define READ_HEADER 0x44
#define GET_EVENT_STATUS_NOTIFICATION 0x4a
#define LOG_SELECT 0x4c
#define LOG_SENSE 0x4d
#define XDWRITEREAD_10 0x53
#define MODE_SELECT_10 0x55
#define RESERVE_10 0x56
#define RELEASE_10 0x57
#define MODE_SENSE_10 0x5a
#define PERSISTENT_RESERVE_IN 0x5e
#define PERSISTENT_RESERVE_OUT 0x5f
#define VARIABLE_LENGTH_CMD 0x7f
#define REPORT_LUNS 0xa0
#define SECURITY_PROTOCOL_IN 0xa2
#define MAINTENANCE_IN 0xa3
#define MAINTENANCE_OUT 0xa4
#define MOVE_MEDIUM 0xa5
#define EXCHANGE_MEDIUM 0xa6
#define READ_12 0xa8
#define SERVICE_ACTION_OUT_12 0xa9
#define WRITE_12 0xaa
#define READ_MEDIA_SERIAL_NUMBER 0xab
#define SERVICE_ACTION_IN_12 0xab
#define WRITE_VERIFY_12 0xae
#define VERIFY_12 0xaf
#define SEARCH_HIGH_12 0xb0
#define SEARCH_EQUAL_12 0xb1
#define SEARCH_LOW_12 0xb2
#define SECURITY_PROTOCOL_OUT 0xb5
#define READ_ELEMENT_STATUS 0xb8
#define SEND_VOLUME_TAG 0xb6
#define WRITE_LONG_2 0xea
#define EXTENDED_COPY 0x83
#define RECEIVE_COPY_RESULTS 0x84
#define ACCESS_CONTROL_IN 0x86
#define ACCESS_CONTROL_OUT 0x87
#define READ_16 0x88
#define COMPARE_AND_WRITE 0x89
#define WRITE_16 0x8a
#define READ_ATTRIBUTE 0x8c
#define WRITE_ATTRIBUTE 0x8d
#define WRITE_VERIFY_16 0x8e
#define VERIFY_16 0x8f
#define SYNCHRONIZE_CACHE_16 0x91
#define WRITE_SAME_16 0x93
#define ZBC_OUT 0x94
#define ZBC_IN 0x95
#define SERVICE_ACTION_BIDIRECTIONAL 0x9d
#define SERVICE_ACTION_IN_16 0x9e
#define SERVICE_ACTION_OUT_16 0x9f
#define GOOD 0x00
#define CHECK_CONDITION 0x01
#define CONDITION_GOOD 0x02
#define BUSY 0x04
#define INTERMEDIATE_GOOD 0x08
#define INTERMEDIATE_C_GOOD 0x0a
#define RESERVATION_CONFLICT 0x0c
#define COMMAND_TERMINATED 0x11
#define QUEUE_FULL 0x14
#define ACA_ACTIVE 0x18
#define TASK_ABORTED 0x20
#define STATUS_MASK 0xfe
#define NO_SENSE 0x00
#define RECOVERED_ERROR 0x01
#define NOT_READY 0x02
#define MEDIUM_ERROR 0x03
#define HARDWARE_ERROR 0x04
#define ILLEGAL_REQUEST 0x05
#define UNIT_ATTENTION 0x06
#define DATA_PROTECT 0x07
#define BLANK_CHECK 0x08
#define COPY_ABORTED 0x0a
#define ABORTED_COMMAND 0x0b
#define VOLUME_OVERFLOW 0x0d
#define MISCOMPARE 0x0e
#define TYPE_DISK 0x00
#define TYPE_TAPE 0x01
#define TYPE_PRINTER 0x02
#define TYPE_PROCESSOR 0x03
#define TYPE_WORM 0x04
#define TYPE_ROM 0x05
#define TYPE_SCANNER 0x06
#define TYPE_MOD 0x07
#define TYPE_MEDIUM_CHANGER 0x08
#define TYPE_COMM 0x09
#define TYPE_RAID 0x0c
#define TYPE_ENCLOSURE 0x0d
#define TYPE_RBC 0x0e
#define TYPE_OSD 0x11
#define TYPE_ZBC 0x14
#define TYPE_WLUN 0x1e
#define TYPE_NO_LUN 0x7f
#define SCSI_ACCESS_STATE_OPTIMAL 0x00
#define SCSI_ACCESS_STATE_ACTIVE 0x01
#define SCSI_ACCESS_STATE_STANDBY 0x02
#define SCSI_ACCESS_STATE_UNAVAILABLE 0x03
#define SCSI_ACCESS_STATE_LBA 0x04
#define SCSI_ACCESS_STATE_OFFLINE 0x0e
#define SCSI_ACCESS_STATE_TRANSITIONING 0x0f
#define SCSI_ACCESS_STATE_MASK 0x0f
#define SCSI_ACCESS_STATE_PREFERRED 0x80
enum zbc_zone_reporting_options {
  ZBC_ZONE_REPORTING_OPTION_ALL = 0,
  ZBC_ZONE_REPORTING_OPTION_EMPTY,
  ZBC_ZONE_REPORTING_OPTION_IMPLICIT_OPEN,
  ZBC_ZONE_REPORTING_OPTION_EXPLICIT_OPEN,
  ZBC_ZONE_REPORTING_OPTION_CLOSED,
  ZBC_ZONE_REPORTING_OPTION_FULL,
  ZBC_ZONE_REPORTING_OPTION_READONLY,
  ZBC_ZONE_REPORTING_OPTION_OFFLINE,
  ZBC_ZONE_REPORTING_OPTION_NEED_RESET_WP = 0x10,
  ZBC_ZONE_REPORTING_OPTION_NON_SEQWRITE,
  ZBC_ZONE_REPORTING_OPTION_NON_WP = 0x3f,
};
#define ZBC_REPORT_ZONE_PARTIAL 0x80
#endif

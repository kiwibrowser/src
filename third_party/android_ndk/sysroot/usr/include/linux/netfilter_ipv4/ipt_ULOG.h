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
#ifndef _IPT_ULOG_H
#define _IPT_ULOG_H
#ifndef NETLINK_NFLOG
#define NETLINK_NFLOG 5
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
#define ULOG_DEFAULT_NLGROUP 1
#define ULOG_DEFAULT_QTHRESHOLD 1
#define ULOG_MAC_LEN 80
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define ULOG_PREFIX_LEN 32
#define ULOG_MAX_QLEN 50
struct ipt_ulog_info {
  unsigned int nl_group;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  size_t copy_range;
  size_t qthreshold;
  char prefix[ULOG_PREFIX_LEN];
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
typedef struct ulog_packet_msg {
  unsigned long mark;
  long timestamp_sec;
  long timestamp_usec;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  unsigned int hook;
  char indev_name[IFNAMSIZ];
  char outdev_name[IFNAMSIZ];
  size_t data_len;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
  char prefix[ULOG_PREFIX_LEN];
  unsigned char mac_len;
  unsigned char mac[ULOG_MAC_LEN];
  unsigned char payload[0];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} ulog_packet_msg_t;
#endif

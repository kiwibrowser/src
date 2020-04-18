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
#ifndef _UAPI_LINUX_AUTO_FS_H
#define _UAPI_LINUX_AUTO_FS_H
#include <linux/types.h>
#include <linux/limits.h>
#include <sys/ioctl.h>
#define AUTOFS_PROTO_VERSION 3
#define AUTOFS_MAX_PROTO_VERSION AUTOFS_PROTO_VERSION
#define AUTOFS_MIN_PROTO_VERSION AUTOFS_PROTO_VERSION
#if defined(__ia64__) || defined(__alpha__)
typedef unsigned long autofs_wqt_t;
#else
typedef unsigned int autofs_wqt_t;
#endif
#define autofs_ptype_missing 0
#define autofs_ptype_expire 1
struct autofs_packet_hdr {
  int proto_version;
  int type;
};
struct autofs_packet_missing {
  struct autofs_packet_hdr hdr;
  autofs_wqt_t wait_queue_token;
  int len;
  char name[NAME_MAX + 1];
};
struct autofs_packet_expire {
  struct autofs_packet_hdr hdr;
  int len;
  char name[NAME_MAX + 1];
};
#define AUTOFS_IOCTL 0x93
enum {
  AUTOFS_IOC_READY_CMD = 0x60,
  AUTOFS_IOC_FAIL_CMD,
  AUTOFS_IOC_CATATONIC_CMD,
  AUTOFS_IOC_PROTOVER_CMD,
  AUTOFS_IOC_SETTIMEOUT_CMD,
  AUTOFS_IOC_EXPIRE_CMD,
};
#define AUTOFS_IOC_READY _IO(AUTOFS_IOCTL, AUTOFS_IOC_READY_CMD)
#define AUTOFS_IOC_FAIL _IO(AUTOFS_IOCTL, AUTOFS_IOC_FAIL_CMD)
#define AUTOFS_IOC_CATATONIC _IO(AUTOFS_IOCTL, AUTOFS_IOC_CATATONIC_CMD)
#define AUTOFS_IOC_PROTOVER _IOR(AUTOFS_IOCTL, AUTOFS_IOC_PROTOVER_CMD, int)
#define AUTOFS_IOC_SETTIMEOUT32 _IOWR(AUTOFS_IOCTL, AUTOFS_IOC_SETTIMEOUT_CMD, compat_ulong_t)
#define AUTOFS_IOC_SETTIMEOUT _IOWR(AUTOFS_IOCTL, AUTOFS_IOC_SETTIMEOUT_CMD, unsigned long)
#define AUTOFS_IOC_EXPIRE _IOR(AUTOFS_IOCTL, AUTOFS_IOC_EXPIRE_CMD, struct autofs_packet_expire)
#endif

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
#ifndef _LINUX_AUTO_FS4_H
#define _LINUX_AUTO_FS4_H
#include <linux/types.h>
#include <linux/auto_fs.h>
#undef AUTOFS_PROTO_VERSION
#undef AUTOFS_MIN_PROTO_VERSION
#undef AUTOFS_MAX_PROTO_VERSION
#define AUTOFS_PROTO_VERSION 5
#define AUTOFS_MIN_PROTO_VERSION 3
#define AUTOFS_MAX_PROTO_VERSION 5
#define AUTOFS_PROTO_SUBVERSION 2
#define AUTOFS_EXP_IMMEDIATE 1
#define AUTOFS_EXP_LEAVES 2
#define AUTOFS_TYPE_ANY 0U
#define AUTOFS_TYPE_INDIRECT 1U
#define AUTOFS_TYPE_DIRECT 2U
#define AUTOFS_TYPE_OFFSET 4U
enum autofs_notify {
  NFY_NONE,
  NFY_MOUNT,
  NFY_EXPIRE
};
#define autofs_ptype_expire_multi 2
#define autofs_ptype_missing_indirect 3
#define autofs_ptype_expire_indirect 4
#define autofs_ptype_missing_direct 5
#define autofs_ptype_expire_direct 6
struct autofs_packet_expire_multi {
  struct autofs_packet_hdr hdr;
  autofs_wqt_t wait_queue_token;
  int len;
  char name[NAME_MAX + 1];
};
union autofs_packet_union {
  struct autofs_packet_hdr hdr;
  struct autofs_packet_missing missing;
  struct autofs_packet_expire expire;
  struct autofs_packet_expire_multi expire_multi;
};
struct autofs_v5_packet {
  struct autofs_packet_hdr hdr;
  autofs_wqt_t wait_queue_token;
  __u32 dev;
  __u64 ino;
  __u32 uid;
  __u32 gid;
  __u32 pid;
  __u32 tgid;
  __u32 len;
  char name[NAME_MAX + 1];
};
typedef struct autofs_v5_packet autofs_packet_missing_indirect_t;
typedef struct autofs_v5_packet autofs_packet_expire_indirect_t;
typedef struct autofs_v5_packet autofs_packet_missing_direct_t;
typedef struct autofs_v5_packet autofs_packet_expire_direct_t;
union autofs_v5_packet_union {
  struct autofs_packet_hdr hdr;
  struct autofs_v5_packet v5_packet;
  autofs_packet_missing_indirect_t missing_indirect;
  autofs_packet_expire_indirect_t expire_indirect;
  autofs_packet_missing_direct_t missing_direct;
  autofs_packet_expire_direct_t expire_direct;
};
enum {
  AUTOFS_IOC_EXPIRE_MULTI_CMD = 0x66,
  AUTOFS_IOC_PROTOSUBVER_CMD,
  AUTOFS_IOC_ASKUMOUNT_CMD = 0x70,
};
#define AUTOFS_IOC_EXPIRE_MULTI _IOW(AUTOFS_IOCTL, AUTOFS_IOC_EXPIRE_MULTI_CMD, int)
#define AUTOFS_IOC_EXPIRE_INDIRECT AUTOFS_IOC_EXPIRE_MULTI
#define AUTOFS_IOC_EXPIRE_DIRECT AUTOFS_IOC_EXPIRE_MULTI
#define AUTOFS_IOC_PROTOSUBVER _IOR(AUTOFS_IOCTL, AUTOFS_IOC_PROTOSUBVER_CMD, int)
#define AUTOFS_IOC_ASKUMOUNT _IOR(AUTOFS_IOCTL, AUTOFS_IOC_ASKUMOUNT_CMD, int)
#endif

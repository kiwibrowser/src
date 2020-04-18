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
#ifndef _LINUX_VHOST_H
#define _LINUX_VHOST_H
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/ioctl.h>
#include <linux/virtio_config.h>
#include <linux/virtio_ring.h>
struct vhost_vring_state {
  unsigned int index;
  unsigned int num;
};
struct vhost_vring_file {
  unsigned int index;
  int fd;
};
struct vhost_vring_addr {
  unsigned int index;
  unsigned int flags;
#define VHOST_VRING_F_LOG 0
  __u64 desc_user_addr;
  __u64 used_user_addr;
  __u64 avail_user_addr;
  __u64 log_guest_addr;
};
struct vhost_iotlb_msg {
  __u64 iova;
  __u64 size;
  __u64 uaddr;
#define VHOST_ACCESS_RO 0x1
#define VHOST_ACCESS_WO 0x2
#define VHOST_ACCESS_RW 0x3
  __u8 perm;
#define VHOST_IOTLB_MISS 1
#define VHOST_IOTLB_UPDATE 2
#define VHOST_IOTLB_INVALIDATE 3
#define VHOST_IOTLB_ACCESS_FAIL 4
  __u8 type;
};
#define VHOST_IOTLB_MSG 0x1
struct vhost_msg {
  int type;
  union {
    struct vhost_iotlb_msg iotlb;
    __u8 padding[64];
  };
};
struct vhost_memory_region {
  __u64 guest_phys_addr;
  __u64 memory_size;
  __u64 userspace_addr;
  __u64 flags_padding;
};
#define VHOST_PAGE_SIZE 0x1000
struct vhost_memory {
  __u32 nregions;
  __u32 padding;
  struct vhost_memory_region regions[0];
};
#define VHOST_VIRTIO 0xAF
#define VHOST_GET_FEATURES _IOR(VHOST_VIRTIO, 0x00, __u64)
#define VHOST_SET_FEATURES _IOW(VHOST_VIRTIO, 0x00, __u64)
#define VHOST_SET_OWNER _IO(VHOST_VIRTIO, 0x01)
#define VHOST_RESET_OWNER _IO(VHOST_VIRTIO, 0x02)
#define VHOST_SET_MEM_TABLE _IOW(VHOST_VIRTIO, 0x03, struct vhost_memory)
#define VHOST_SET_LOG_BASE _IOW(VHOST_VIRTIO, 0x04, __u64)
#define VHOST_SET_LOG_FD _IOW(VHOST_VIRTIO, 0x07, int)
#define VHOST_SET_VRING_NUM _IOW(VHOST_VIRTIO, 0x10, struct vhost_vring_state)
#define VHOST_SET_VRING_ADDR _IOW(VHOST_VIRTIO, 0x11, struct vhost_vring_addr)
#define VHOST_SET_VRING_BASE _IOW(VHOST_VIRTIO, 0x12, struct vhost_vring_state)
#define VHOST_GET_VRING_BASE _IOWR(VHOST_VIRTIO, 0x12, struct vhost_vring_state)
#define VHOST_VRING_LITTLE_ENDIAN 0
#define VHOST_VRING_BIG_ENDIAN 1
#define VHOST_SET_VRING_ENDIAN _IOW(VHOST_VIRTIO, 0x13, struct vhost_vring_state)
#define VHOST_GET_VRING_ENDIAN _IOW(VHOST_VIRTIO, 0x14, struct vhost_vring_state)
#define VHOST_SET_VRING_KICK _IOW(VHOST_VIRTIO, 0x20, struct vhost_vring_file)
#define VHOST_SET_VRING_CALL _IOW(VHOST_VIRTIO, 0x21, struct vhost_vring_file)
#define VHOST_SET_VRING_ERR _IOW(VHOST_VIRTIO, 0x22, struct vhost_vring_file)
#define VHOST_SET_VRING_BUSYLOOP_TIMEOUT _IOW(VHOST_VIRTIO, 0x23, struct vhost_vring_state)
#define VHOST_GET_VRING_BUSYLOOP_TIMEOUT _IOW(VHOST_VIRTIO, 0x24, struct vhost_vring_state)
#define VHOST_NET_SET_BACKEND _IOW(VHOST_VIRTIO, 0x30, struct vhost_vring_file)
#define VHOST_F_LOG_ALL 26
#define VHOST_NET_F_VIRTIO_NET_HDR 27
#define VHOST_SCSI_ABI_VERSION 1
struct vhost_scsi_target {
  int abi_version;
  char vhost_wwpn[224];
  unsigned short vhost_tpgt;
  unsigned short reserved;
};
#define VHOST_SCSI_SET_ENDPOINT _IOW(VHOST_VIRTIO, 0x40, struct vhost_scsi_target)
#define VHOST_SCSI_CLEAR_ENDPOINT _IOW(VHOST_VIRTIO, 0x41, struct vhost_scsi_target)
#define VHOST_SCSI_GET_ABI_VERSION _IOW(VHOST_VIRTIO, 0x42, int)
#define VHOST_SCSI_SET_EVENTS_MISSED _IOW(VHOST_VIRTIO, 0x43, __u32)
#define VHOST_SCSI_GET_EVENTS_MISSED _IOW(VHOST_VIRTIO, 0x44, __u32)
#define VHOST_VSOCK_SET_GUEST_CID _IOW(VHOST_VIRTIO, 0x60, __u64)
#define VHOST_VSOCK_SET_RUNNING _IOW(VHOST_VIRTIO, 0x61, int)
#endif

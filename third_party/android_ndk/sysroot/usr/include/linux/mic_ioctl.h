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
#ifndef _MIC_IOCTL_H_
#define _MIC_IOCTL_H_
#include <linux/types.h>
struct mic_copy_desc {
  struct iovec * iov;
  __u32 iovcnt;
  __u8 vr_idx;
  __u8 update_used;
  __u32 out_len;
};
#define MIC_VIRTIO_ADD_DEVICE _IOWR('s', 1, struct mic_device_desc *)
#define MIC_VIRTIO_COPY_DESC _IOWR('s', 2, struct mic_copy_desc *)
#define MIC_VIRTIO_CONFIG_CHANGE _IOWR('s', 5, __u8 *)
#endif

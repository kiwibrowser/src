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
#ifndef _UAPI_LINUX_VIRTIO_RING_H
#define _UAPI_LINUX_VIRTIO_RING_H
#include <stdint.h>
#include <linux/types.h>
#include <linux/virtio_types.h>
#define VRING_DESC_F_NEXT 1
#define VRING_DESC_F_WRITE 2
#define VRING_DESC_F_INDIRECT 4
#define VRING_USED_F_NO_NOTIFY 1
#define VRING_AVAIL_F_NO_INTERRUPT 1
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX 29
struct vring_desc {
  __virtio64 addr;
  __virtio32 len;
  __virtio16 flags;
  __virtio16 next;
};
struct vring_avail {
  __virtio16 flags;
  __virtio16 idx;
  __virtio16 ring[];
};
struct vring_used_elem {
  __virtio32 id;
  __virtio32 len;
};
struct vring_used {
  __virtio16 flags;
  __virtio16 idx;
  struct vring_used_elem ring[];
};
struct vring {
  unsigned int num;
  struct vring_desc * desc;
  struct vring_avail * avail;
  struct vring_used * used;
};
#define VRING_AVAIL_ALIGN_SIZE 2
#define VRING_USED_ALIGN_SIZE 4
#define VRING_DESC_ALIGN_SIZE 16
#define vring_used_event(vr) ((vr)->avail->ring[(vr)->num])
#define vring_avail_event(vr) (* (__virtio16 *) & (vr)->used->ring[(vr)->num])
#endif

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
#ifndef __MIC_COMMON_H_
#define __MIC_COMMON_H_
#include <linux/virtio_ring.h>
#define __mic_align(a,x) (((a) + (x) - 1) & ~((x) - 1))
struct mic_device_desc {
  __s8 type;
  __u8 num_vq;
  __u8 feature_len;
  __u8 config_len;
  __u8 status;
  __le64 config[0];
} __attribute__((aligned(8)));
struct mic_device_ctrl {
  __le64 vdev;
  __u8 config_change;
  __u8 vdev_reset;
  __u8 guest_ack;
  __u8 host_ack;
  __u8 used_address_updated;
  __s8 c2h_vdev_db;
  __s8 h2c_vdev_db;
} __attribute__((aligned(8)));
struct mic_bootparam {
  __le32 magic;
  __s8 h2c_config_db;
  __u8 node_id;
  __u8 h2c_scif_db;
  __u8 c2h_scif_db;
  __u64 scif_host_dma_addr;
  __u64 scif_card_dma_addr;
} __attribute__((aligned(8)));
struct mic_device_page {
  struct mic_bootparam bootparam;
  struct mic_device_desc desc[0];
};
struct mic_vqconfig {
  __le64 address;
  __le64 used_address;
  __le16 num;
} __attribute__((aligned(8)));
#define MIC_VIRTIO_RING_ALIGN 4096
#define MIC_MAX_VRINGS 4
#define MIC_VRING_ENTRIES 128
#define MIC_MAX_VRING_ENTRIES 128
#define MIC_MAX_DESC_BLK_SIZE 256
struct _mic_vring_info {
  __u16 avail_idx;
  __le32 magic;
};
struct mic_vring {
  struct vring vr;
  struct _mic_vring_info * info;
  void * va;
  int len;
};
#define mic_aligned_desc_size(d) __mic_align(mic_desc_size(d), 8)
#ifndef INTEL_MIC_CARD
#endif
#define MIC_DP_SIZE 4096
#define MIC_MAGIC 0xc0ffee00
enum mic_states {
  MIC_READY = 0,
  MIC_BOOTING,
  MIC_ONLINE,
  MIC_SHUTTING_DOWN,
  MIC_RESETTING,
  MIC_RESET_FAILED,
  MIC_LAST
};
enum mic_status {
  MIC_NOP = 0,
  MIC_CRASHED,
  MIC_HALTED,
  MIC_POWER_OFF,
  MIC_RESTART,
  MIC_STATUS_LAST
};
#endif

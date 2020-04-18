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
#ifndef __WIL6210_UAPI_H__
#define __WIL6210_UAPI_H__
#define __user
#include <linux/sockios.h>
#define WIL_IOCTL_MEMIO (SIOCDEVPRIVATE + 2)
#define WIL_IOCTL_MEMIO_BLOCK (SIOCDEVPRIVATE + 3)
enum wil_memio_op {
  wil_mmio_read = 0,
  wil_mmio_write = 1,
  wil_mmio_op_mask = 0xff,
  wil_mmio_addr_linker = 0 << 8,
  wil_mmio_addr_ahb = 1 << 8,
  wil_mmio_addr_bar = 2 << 8,
  wil_mmio_addr_mask = 0xff00,
};
struct wil_memio {
  uint32_t op;
  uint32_t addr;
  uint32_t val;
};
struct wil_memio_block {
  uint32_t op;
  uint32_t addr;
  uint32_t size;
  void __user * block;
};
#endif

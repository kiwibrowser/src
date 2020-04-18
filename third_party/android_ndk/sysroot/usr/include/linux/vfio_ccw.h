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
#ifndef _VFIO_CCW_H_
#define _VFIO_CCW_H_
#include <linux/types.h>
struct ccw_io_region {
#define ORB_AREA_SIZE 12
  __u8 orb_area[ORB_AREA_SIZE];
#define SCSW_AREA_SIZE 12
  __u8 scsw_area[SCSW_AREA_SIZE];
#define IRB_AREA_SIZE 96
  __u8 irb_area[IRB_AREA_SIZE];
  __u32 ret_code;
} __packed;
#endif

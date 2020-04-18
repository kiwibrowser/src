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
#ifndef _XT_DSCP_H
#define _XT_DSCP_H
#include <linux/types.h>
#define XT_DSCP_MASK 0xfc
#define XT_DSCP_SHIFT 2
#define XT_DSCP_MAX 0x3f
struct xt_dscp_info {
  __u8 dscp;
  __u8 invert;
};
struct xt_tos_match_info {
  __u8 tos_mask;
  __u8 tos_value;
  __u8 invert;
};
#endif

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
#ifndef _IP6T_HL_H
#define _IP6T_HL_H
#include <linux/types.h>
enum {
  IP6T_HL_EQ = 0,
  IP6T_HL_NE,
  IP6T_HL_LT,
  IP6T_HL_GT,
};
struct ip6t_hl_info {
  __u8 mode;
  __u8 hop_limit;
};
#endif

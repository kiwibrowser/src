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
#ifndef _XT_RATEEST_MATCH_H
#define _XT_RATEEST_MATCH_H
#include <linux/types.h>
#include <linux/if.h>
enum xt_rateest_match_flags {
  XT_RATEEST_MATCH_INVERT = 1 << 0,
  XT_RATEEST_MATCH_ABS = 1 << 1,
  XT_RATEEST_MATCH_REL = 1 << 2,
  XT_RATEEST_MATCH_DELTA = 1 << 3,
  XT_RATEEST_MATCH_BPS = 1 << 4,
  XT_RATEEST_MATCH_PPS = 1 << 5,
};
enum xt_rateest_match_mode {
  XT_RATEEST_MATCH_NONE,
  XT_RATEEST_MATCH_EQ,
  XT_RATEEST_MATCH_LT,
  XT_RATEEST_MATCH_GT,
};
struct xt_rateest_match_info {
  char name1[IFNAMSIZ];
  char name2[IFNAMSIZ];
  __u16 flags;
  __u16 mode;
  __u32 bps1;
  __u32 pps1;
  __u32 bps2;
  __u32 pps2;
  struct xt_rateest * est1 __attribute__((aligned(8)));
  struct xt_rateest * est2 __attribute__((aligned(8)));
};
#endif

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
#ifndef _IPT_ECN_H
#define _IPT_ECN_H
#include <linux/netfilter/xt_ecn.h>
#define ipt_ecn_info xt_ecn_info
enum {
  IPT_ECN_IP_MASK = XT_ECN_IP_MASK,
  IPT_ECN_OP_MATCH_IP = XT_ECN_OP_MATCH_IP,
  IPT_ECN_OP_MATCH_ECE = XT_ECN_OP_MATCH_ECE,
  IPT_ECN_OP_MATCH_CWR = XT_ECN_OP_MATCH_CWR,
  IPT_ECN_OP_MATCH_MASK = XT_ECN_OP_MATCH_MASK,
};
#endif

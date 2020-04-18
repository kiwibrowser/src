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
#ifndef HNS_ABI_USER_H
#define HNS_ABI_USER_H
#include <linux/types.h>
struct hns_roce_ib_create_cq {
  __u64 buf_addr;
};
struct hns_roce_ib_create_qp {
  __u64 buf_addr;
  __u64 db_addr;
  __u8 log_sq_bb_count;
  __u8 log_sq_stride;
  __u8 sq_no_prefetch;
  __u8 reserved[5];
};
struct hns_roce_ib_alloc_ucontext_resp {
  __u32 qp_tab_size;
};
#endif

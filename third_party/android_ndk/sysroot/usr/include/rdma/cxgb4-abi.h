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
#ifndef CXGB4_ABI_USER_H
#define CXGB4_ABI_USER_H
#include <linux/types.h>
#define C4IW_UVERBS_ABI_VERSION 3
struct c4iw_create_cq_resp {
  __u64 key;
  __u64 gts_key;
  __u64 memsize;
  __u32 cqid;
  __u32 size;
  __u32 qid_mask;
  __u32 reserved;
};
enum {
  C4IW_QPF_ONCHIP = (1 << 0)
};
struct c4iw_create_qp_resp {
  __u64 ma_sync_key;
  __u64 sq_key;
  __u64 rq_key;
  __u64 sq_db_gts_key;
  __u64 rq_db_gts_key;
  __u64 sq_memsize;
  __u64 rq_memsize;
  __u32 sqid;
  __u32 rqid;
  __u32 sq_size;
  __u32 rq_size;
  __u32 qid_mask;
  __u32 flags;
};
struct c4iw_alloc_ucontext_resp {
  __u64 status_page_key;
  __u32 status_page_size;
  __u32 reserved;
};
#endif

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
#ifndef __BNXT_RE_UVERBS_ABI_H__
#define __BNXT_RE_UVERBS_ABI_H__
#include <linux/types.h>
#define BNXT_RE_ABI_VERSION 1
struct bnxt_re_uctx_resp {
  __u32 dev_id;
  __u32 max_qp;
  __u32 pg_size;
  __u32 cqe_sz;
  __u32 max_cqd;
  __u32 rsvd;
};
struct bnxt_re_pd_resp {
  __u32 pdid;
  __u32 dpi;
  __u64 dbr;
};
struct bnxt_re_cq_req {
  __u64 cq_va;
  __u64 cq_handle;
};
struct bnxt_re_cq_resp {
  __u32 cqid;
  __u32 tail;
  __u32 phase;
  __u32 rsvd;
};
struct bnxt_re_qp_req {
  __u64 qpsva;
  __u64 qprva;
  __u64 qp_handle;
};
struct bnxt_re_qp_resp {
  __u32 qpid;
  __u32 rsvd;
};
enum bnxt_re_shpg_offt {
  BNXT_RE_BEG_RESV_OFFT = 0x00,
  BNXT_RE_AVID_OFFT = 0x10,
  BNXT_RE_AVID_SIZE = 0x04,
  BNXT_RE_END_RESV_OFFT = 0xFF0
};
#endif

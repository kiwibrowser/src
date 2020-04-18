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
#ifndef RDMA_USER_RXE_H
#define RDMA_USER_RXE_H
#include <linux/types.h>
union rxe_gid {
  __u8 raw[16];
  struct {
    __be64 subnet_prefix;
    __be64 interface_id;
  } global;
};
struct rxe_global_route {
  union rxe_gid dgid;
  __u32 flow_label;
  __u8 sgid_index;
  __u8 hop_limit;
  __u8 traffic_class;
};
struct rxe_av {
  __u8 port_num;
  __u8 network_type;
  struct rxe_global_route grh;
  union {
    struct sockaddr _sockaddr;
    struct sockaddr_in _sockaddr_in;
    struct sockaddr_in6 _sockaddr_in6;
  } sgid_addr, dgid_addr;
};
struct rxe_send_wr {
  __u64 wr_id;
  __u32 num_sge;
  __u32 opcode;
  __u32 send_flags;
  union {
    __be32 imm_data;
    __u32 invalidate_rkey;
  } ex;
  union {
    struct {
      __u64 remote_addr;
      __u32 rkey;
    } rdma;
    struct {
      __u64 remote_addr;
      __u64 compare_add;
      __u64 swap;
      __u32 rkey;
    } atomic;
    struct {
      __u32 remote_qpn;
      __u32 remote_qkey;
      __u16 pkey_index;
    } ud;
    struct {
      struct ib_mr * mr;
      __u32 key;
      int access;
    } reg;
  } wr;
};
struct rxe_sge {
  __u64 addr;
  __u32 length;
  __u32 lkey;
};
struct mminfo {
  __u64 offset;
  __u32 size;
  __u32 pad;
};
struct rxe_dma_info {
  __u32 length;
  __u32 resid;
  __u32 cur_sge;
  __u32 num_sge;
  __u32 sge_offset;
  union {
    __u8 inline_data[0];
    struct rxe_sge sge[0];
  };
};
struct rxe_send_wqe {
  struct rxe_send_wr wr;
  struct rxe_av av;
  __u32 status;
  __u32 state;
  __u64 iova;
  __u32 mask;
  __u32 first_psn;
  __u32 last_psn;
  __u32 ack_length;
  __u32 ssn;
  __u32 has_rd_atomic;
  struct rxe_dma_info dma;
};
struct rxe_recv_wqe {
  __u64 wr_id;
  __u32 num_sge;
  __u32 padding;
  struct rxe_dma_info dma;
};
#endif

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
#ifndef MLX5_ABI_USER_H
#define MLX5_ABI_USER_H
#include <linux/types.h>
#include <linux/if_ether.h>
enum {
  MLX5_QP_FLAG_SIGNATURE = 1 << 0,
  MLX5_QP_FLAG_SCATTER_CQE = 1 << 1,
};
enum {
  MLX5_SRQ_FLAG_SIGNATURE = 1 << 0,
};
enum {
  MLX5_WQ_FLAG_SIGNATURE = 1 << 0,
};
#define MLX5_IB_UVERBS_ABI_VERSION 1
struct mlx5_ib_alloc_ucontext_req {
  __u32 total_num_bfregs;
  __u32 num_low_latency_bfregs;
};
enum mlx5_lib_caps {
  MLX5_LIB_CAP_4K_UAR = (__u64) 1 << 0,
};
struct mlx5_ib_alloc_ucontext_req_v2 {
  __u32 total_num_bfregs;
  __u32 num_low_latency_bfregs;
  __u32 flags;
  __u32 comp_mask;
  __u8 max_cqe_version;
  __u8 reserved0;
  __u16 reserved1;
  __u32 reserved2;
  __u64 lib_caps;
};
enum mlx5_ib_alloc_ucontext_resp_mask {
  MLX5_IB_ALLOC_UCONTEXT_RESP_MASK_CORE_CLOCK_OFFSET = 1UL << 0,
};
enum mlx5_user_cmds_supp_uhw {
  MLX5_USER_CMDS_SUPP_UHW_QUERY_DEVICE = 1 << 0,
  MLX5_USER_CMDS_SUPP_UHW_CREATE_AH = 1 << 1,
};
enum mlx5_user_inline_mode {
  MLX5_USER_INLINE_MODE_NA,
  MLX5_USER_INLINE_MODE_NONE,
  MLX5_USER_INLINE_MODE_L2,
  MLX5_USER_INLINE_MODE_IP,
  MLX5_USER_INLINE_MODE_TCP_UDP,
};
struct mlx5_ib_alloc_ucontext_resp {
  __u32 qp_tab_size;
  __u32 bf_reg_size;
  __u32 tot_bfregs;
  __u32 cache_line_size;
  __u16 max_sq_desc_sz;
  __u16 max_rq_desc_sz;
  __u32 max_send_wqebb;
  __u32 max_recv_wr;
  __u32 max_srq_recv_wr;
  __u16 num_ports;
  __u16 reserved1;
  __u32 comp_mask;
  __u32 response_length;
  __u8 cqe_version;
  __u8 cmds_supp_uhw;
  __u8 eth_min_inline;
  __u8 reserved2;
  __u64 hca_core_clock_offset;
  __u32 log_uar_size;
  __u32 num_uars_per_page;
};
struct mlx5_ib_alloc_pd_resp {
  __u32 pdn;
};
struct mlx5_ib_tso_caps {
  __u32 max_tso;
  __u32 supported_qpts;
};
struct mlx5_ib_rss_caps {
  __u64 rx_hash_fields_mask;
  __u8 rx_hash_function;
  __u8 reserved[7];
};
enum mlx5_ib_cqe_comp_res_format {
  MLX5_IB_CQE_RES_FORMAT_HASH = 1 << 0,
  MLX5_IB_CQE_RES_FORMAT_CSUM = 1 << 1,
  MLX5_IB_CQE_RES_RESERVED = 1 << 2,
};
struct mlx5_ib_cqe_comp_caps {
  __u32 max_num;
  __u32 supported_format;
};
struct mlx5_packet_pacing_caps {
  __u32 qp_rate_limit_min;
  __u32 qp_rate_limit_max;
  __u32 supported_qpts;
  __u32 reserved;
};
struct mlx5_ib_query_device_resp {
  __u32 comp_mask;
  __u32 response_length;
  struct mlx5_ib_tso_caps tso_caps;
  struct mlx5_ib_rss_caps rss_caps;
  struct mlx5_ib_cqe_comp_caps cqe_comp_caps;
  struct mlx5_packet_pacing_caps packet_pacing_caps;
  __u32 mlx5_ib_support_multi_pkt_send_wqes;
  __u32 reserved;
};
struct mlx5_ib_create_cq {
  __u64 buf_addr;
  __u64 db_addr;
  __u32 cqe_size;
  __u8 cqe_comp_en;
  __u8 cqe_comp_res_format;
  __u16 reserved;
};
struct mlx5_ib_create_cq_resp {
  __u32 cqn;
  __u32 reserved;
};
struct mlx5_ib_resize_cq {
  __u64 buf_addr;
  __u16 cqe_size;
  __u16 reserved0;
  __u32 reserved1;
};
struct mlx5_ib_create_srq {
  __u64 buf_addr;
  __u64 db_addr;
  __u32 flags;
  __u32 reserved0;
  __u32 uidx;
  __u32 reserved1;
};
struct mlx5_ib_create_srq_resp {
  __u32 srqn;
  __u32 reserved;
};
struct mlx5_ib_create_qp {
  __u64 buf_addr;
  __u64 db_addr;
  __u32 sq_wqe_count;
  __u32 rq_wqe_count;
  __u32 rq_wqe_shift;
  __u32 flags;
  __u32 uidx;
  __u32 reserved0;
  __u64 sq_buf_addr;
};
enum mlx5_rx_hash_function_flags {
  MLX5_RX_HASH_FUNC_TOEPLITZ = 1 << 0,
};
enum mlx5_rx_hash_fields {
  MLX5_RX_HASH_SRC_IPV4 = 1 << 0,
  MLX5_RX_HASH_DST_IPV4 = 1 << 1,
  MLX5_RX_HASH_SRC_IPV6 = 1 << 2,
  MLX5_RX_HASH_DST_IPV6 = 1 << 3,
  MLX5_RX_HASH_SRC_PORT_TCP = 1 << 4,
  MLX5_RX_HASH_DST_PORT_TCP = 1 << 5,
  MLX5_RX_HASH_SRC_PORT_UDP = 1 << 6,
  MLX5_RX_HASH_DST_PORT_UDP = 1 << 7
};
struct mlx5_ib_create_qp_rss {
  __u64 rx_hash_fields_mask;
  __u8 rx_hash_function;
  __u8 rx_key_len;
  __u8 reserved[6];
  __u8 rx_hash_key[128];
  __u32 comp_mask;
  __u32 reserved1;
};
struct mlx5_ib_create_qp_resp {
  __u32 bfreg_index;
};
struct mlx5_ib_alloc_mw {
  __u32 comp_mask;
  __u8 num_klms;
  __u8 reserved1;
  __u16 reserved2;
};
struct mlx5_ib_create_wq {
  __u64 buf_addr;
  __u64 db_addr;
  __u32 rq_wqe_count;
  __u32 rq_wqe_shift;
  __u32 user_index;
  __u32 flags;
  __u32 comp_mask;
  __u32 reserved;
};
struct mlx5_ib_create_ah_resp {
  __u32 response_length;
  __u8 dmac[ETH_ALEN];
  __u8 reserved[6];
};
struct mlx5_ib_create_wq_resp {
  __u32 response_length;
  __u32 reserved;
};
struct mlx5_ib_create_rwq_ind_tbl_resp {
  __u32 response_length;
  __u32 reserved;
};
struct mlx5_ib_modify_wq {
  __u32 comp_mask;
  __u32 reserved;
};
#endif

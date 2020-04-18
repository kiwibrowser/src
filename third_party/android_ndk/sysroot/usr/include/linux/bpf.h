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
#ifndef _UAPI__LINUX_BPF_H__
#define _UAPI__LINUX_BPF_H__
#include <linux/types.h>
#include <linux/bpf_common.h>
#define BPF_ALU64 0x07
#define BPF_DW 0x18
#define BPF_XADD 0xc0
#define BPF_MOV 0xb0
#define BPF_ARSH 0xc0
#define BPF_END 0xd0
#define BPF_TO_LE 0x00
#define BPF_TO_BE 0x08
#define BPF_FROM_LE BPF_TO_LE
#define BPF_FROM_BE BPF_TO_BE
#define BPF_JNE 0x50
#define BPF_JSGT 0x60
#define BPF_JSGE 0x70
#define BPF_CALL 0x80
#define BPF_EXIT 0x90
enum {
  BPF_REG_0 = 0,
  BPF_REG_1,
  BPF_REG_2,
  BPF_REG_3,
  BPF_REG_4,
  BPF_REG_5,
  BPF_REG_6,
  BPF_REG_7,
  BPF_REG_8,
  BPF_REG_9,
  BPF_REG_10,
  __MAX_BPF_REG,
};
#define MAX_BPF_REG __MAX_BPF_REG
struct bpf_insn {
  __u8 code;
  __u8 dst_reg : 4;
  __u8 src_reg : 4;
  __s16 off;
  __s32 imm;
};
struct bpf_lpm_trie_key {
  __u32 prefixlen;
  __u8 data[0];
};
enum bpf_cmd {
  BPF_MAP_CREATE,
  BPF_MAP_LOOKUP_ELEM,
  BPF_MAP_UPDATE_ELEM,
  BPF_MAP_DELETE_ELEM,
  BPF_MAP_GET_NEXT_KEY,
  BPF_PROG_LOAD,
  BPF_OBJ_PIN,
  BPF_OBJ_GET,
  BPF_PROG_ATTACH,
  BPF_PROG_DETACH,
  BPF_PROG_TEST_RUN,
};
enum bpf_map_type {
  BPF_MAP_TYPE_UNSPEC,
  BPF_MAP_TYPE_HASH,
  BPF_MAP_TYPE_ARRAY,
  BPF_MAP_TYPE_PROG_ARRAY,
  BPF_MAP_TYPE_PERF_EVENT_ARRAY,
  BPF_MAP_TYPE_PERCPU_HASH,
  BPF_MAP_TYPE_PERCPU_ARRAY,
  BPF_MAP_TYPE_STACK_TRACE,
  BPF_MAP_TYPE_CGROUP_ARRAY,
  BPF_MAP_TYPE_LRU_HASH,
  BPF_MAP_TYPE_LRU_PERCPU_HASH,
  BPF_MAP_TYPE_LPM_TRIE,
  BPF_MAP_TYPE_ARRAY_OF_MAPS,
  BPF_MAP_TYPE_HASH_OF_MAPS,
};
enum bpf_prog_type {
  BPF_PROG_TYPE_UNSPEC,
  BPF_PROG_TYPE_SOCKET_FILTER,
  BPF_PROG_TYPE_KPROBE,
  BPF_PROG_TYPE_SCHED_CLS,
  BPF_PROG_TYPE_SCHED_ACT,
  BPF_PROG_TYPE_TRACEPOINT,
  BPF_PROG_TYPE_XDP,
  BPF_PROG_TYPE_PERF_EVENT,
  BPF_PROG_TYPE_CGROUP_SKB,
  BPF_PROG_TYPE_CGROUP_SOCK,
  BPF_PROG_TYPE_LWT_IN,
  BPF_PROG_TYPE_LWT_OUT,
  BPF_PROG_TYPE_LWT_XMIT,
};
enum bpf_attach_type {
  BPF_CGROUP_INET_INGRESS,
  BPF_CGROUP_INET_EGRESS,
  BPF_CGROUP_INET_SOCK_CREATE,
  __MAX_BPF_ATTACH_TYPE
};
#define MAX_BPF_ATTACH_TYPE __MAX_BPF_ATTACH_TYPE
#define BPF_F_ALLOW_OVERRIDE (1U << 0)
#define BPF_F_STRICT_ALIGNMENT (1U << 0)
#define BPF_PSEUDO_MAP_FD 1
#define BPF_ANY 0
#define BPF_NOEXIST 1
#define BPF_EXIST 2
#define BPF_F_NO_PREALLOC (1U << 0)
#define BPF_F_NO_COMMON_LRU (1U << 1)
union bpf_attr {
  struct {
    __u32 map_type;
    __u32 key_size;
    __u32 value_size;
    __u32 max_entries;
    __u32 map_flags;
    __u32 inner_map_fd;
  };
  struct {
    __u32 map_fd;
    __aligned_u64 key;
    union {
      __aligned_u64 value;
      __aligned_u64 next_key;
    };
    __u64 flags;
  };
  struct {
    __u32 prog_type;
    __u32 insn_cnt;
    __aligned_u64 insns;
    __aligned_u64 license;
    __u32 log_level;
    __u32 log_size;
    __aligned_u64 log_buf;
    __u32 kern_version;
    __u32 prog_flags;
  };
  struct {
    __aligned_u64 pathname;
    __u32 bpf_fd;
  };
  struct {
    __u32 target_fd;
    __u32 attach_bpf_fd;
    __u32 attach_type;
    __u32 attach_flags;
  };
  struct {
    __u32 prog_fd;
    __u32 retval;
    __u32 data_size_in;
    __u32 data_size_out;
    __aligned_u64 data_in;
    __aligned_u64 data_out;
    __u32 repeat;
    __u32 duration;
  } test;
} __attribute__((aligned(8)));
#define __BPF_FUNC_MAPPER(FN) FN(unspec), FN(map_lookup_elem), FN(map_update_elem), FN(map_delete_elem), FN(probe_read), FN(ktime_get_ns), FN(trace_printk), FN(get_prandom_u32), FN(get_smp_processor_id), FN(skb_store_bytes), FN(l3_csum_replace), FN(l4_csum_replace), FN(tail_call), FN(clone_redirect), FN(get_current_pid_tgid), FN(get_current_uid_gid), FN(get_current_comm), FN(get_cgroup_classid), FN(skb_vlan_push), FN(skb_vlan_pop), FN(skb_get_tunnel_key), FN(skb_set_tunnel_key), FN(perf_event_read), FN(redirect), FN(get_route_realm), FN(perf_event_output), FN(skb_load_bytes), FN(get_stackid), FN(csum_diff), FN(skb_get_tunnel_opt), FN(skb_set_tunnel_opt), FN(skb_change_proto), FN(skb_change_type), FN(skb_under_cgroup), FN(get_hash_recalc), FN(get_current_task), FN(probe_write_user), FN(current_task_under_cgroup), FN(skb_change_tail), FN(skb_pull_data), FN(csum_update), FN(set_hash_invalid), FN(get_numa_node_id), FN(skb_change_head), FN(xdp_adjust_head), FN(probe_read_str), FN(get_socket_cookie), FN(get_socket_uid),
#define __BPF_ENUM_FN(x) BPF_FUNC_ ##x
enum bpf_func_id {
  __BPF_FUNC_MAPPER(__BPF_ENUM_FN) __BPF_FUNC_MAX_ID,
};
#undef __BPF_ENUM_FN
#define BPF_F_RECOMPUTE_CSUM (1ULL << 0)
#define BPF_F_INVALIDATE_HASH (1ULL << 1)
#define BPF_F_HDR_FIELD_MASK 0xfULL
#define BPF_F_PSEUDO_HDR (1ULL << 4)
#define BPF_F_MARK_MANGLED_0 (1ULL << 5)
#define BPF_F_MARK_ENFORCE (1ULL << 6)
#define BPF_F_INGRESS (1ULL << 0)
#define BPF_F_TUNINFO_IPV6 (1ULL << 0)
#define BPF_F_SKIP_FIELD_MASK 0xffULL
#define BPF_F_USER_STACK (1ULL << 8)
#define BPF_F_FAST_STACK_CMP (1ULL << 9)
#define BPF_F_REUSE_STACKID (1ULL << 10)
#define BPF_F_ZERO_CSUM_TX (1ULL << 1)
#define BPF_F_DONT_FRAGMENT (1ULL << 2)
#define BPF_F_INDEX_MASK 0xffffffffULL
#define BPF_F_CURRENT_CPU BPF_F_INDEX_MASK
#define BPF_F_CTXLEN_MASK (0xfffffULL << 32)
struct __sk_buff {
  __u32 len;
  __u32 pkt_type;
  __u32 mark;
  __u32 queue_mapping;
  __u32 protocol;
  __u32 vlan_present;
  __u32 vlan_tci;
  __u32 vlan_proto;
  __u32 priority;
  __u32 ingress_ifindex;
  __u32 ifindex;
  __u32 tc_index;
  __u32 cb[5];
  __u32 hash;
  __u32 tc_classid;
  __u32 data;
  __u32 data_end;
  __u32 napi_id;
};
struct bpf_tunnel_key {
  __u32 tunnel_id;
  union {
    __u32 remote_ipv4;
    __u32 remote_ipv6[4];
  };
  __u8 tunnel_tos;
  __u8 tunnel_ttl;
  __u16 tunnel_ext;
  __u32 tunnel_label;
};
enum bpf_ret_code {
  BPF_OK = 0,
  BPF_DROP = 2,
  BPF_REDIRECT = 7,
};
struct bpf_sock {
  __u32 bound_dev_if;
  __u32 family;
  __u32 type;
  __u32 protocol;
};
#define XDP_PACKET_HEADROOM 256
enum xdp_action {
  XDP_ABORTED = 0,
  XDP_DROP,
  XDP_PASS,
  XDP_TX,
};
struct xdp_md {
  __u32 data;
  __u32 data_end;
};
#endif

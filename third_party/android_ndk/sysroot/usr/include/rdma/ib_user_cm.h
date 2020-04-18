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
#ifndef IB_USER_CM_H
#define IB_USER_CM_H
#include <linux/types.h>
#include <rdma/ib_user_sa.h>
#define IB_USER_CM_ABI_VERSION 5
enum {
  IB_USER_CM_CMD_CREATE_ID,
  IB_USER_CM_CMD_DESTROY_ID,
  IB_USER_CM_CMD_ATTR_ID,
  IB_USER_CM_CMD_LISTEN,
  IB_USER_CM_CMD_NOTIFY,
  IB_USER_CM_CMD_SEND_REQ,
  IB_USER_CM_CMD_SEND_REP,
  IB_USER_CM_CMD_SEND_RTU,
  IB_USER_CM_CMD_SEND_DREQ,
  IB_USER_CM_CMD_SEND_DREP,
  IB_USER_CM_CMD_SEND_REJ,
  IB_USER_CM_CMD_SEND_MRA,
  IB_USER_CM_CMD_SEND_LAP,
  IB_USER_CM_CMD_SEND_APR,
  IB_USER_CM_CMD_SEND_SIDR_REQ,
  IB_USER_CM_CMD_SEND_SIDR_REP,
  IB_USER_CM_CMD_EVENT,
  IB_USER_CM_CMD_INIT_QP_ATTR,
};
struct ib_ucm_cmd_hdr {
  __u32 cmd;
  __u16 in;
  __u16 out;
};
struct ib_ucm_create_id {
  __u64 uid;
  __u64 response;
};
struct ib_ucm_create_id_resp {
  __u32 id;
};
struct ib_ucm_destroy_id {
  __u64 response;
  __u32 id;
  __u32 reserved;
};
struct ib_ucm_destroy_id_resp {
  __u32 events_reported;
};
struct ib_ucm_attr_id {
  __u64 response;
  __u32 id;
  __u32 reserved;
};
struct ib_ucm_attr_id_resp {
  __be64 service_id;
  __be64 service_mask;
  __be32 local_id;
  __be32 remote_id;
};
struct ib_ucm_init_qp_attr {
  __u64 response;
  __u32 id;
  __u32 qp_state;
};
struct ib_ucm_listen {
  __be64 service_id;
  __be64 service_mask;
  __u32 id;
  __u32 reserved;
};
struct ib_ucm_notify {
  __u32 id;
  __u32 event;
};
struct ib_ucm_private_data {
  __u64 data;
  __u32 id;
  __u8 len;
  __u8 reserved[3];
};
struct ib_ucm_req {
  __u32 id;
  __u32 qpn;
  __u32 qp_type;
  __u32 psn;
  __be64 sid;
  __u64 data;
  __u64 primary_path;
  __u64 alternate_path;
  __u8 len;
  __u8 peer_to_peer;
  __u8 responder_resources;
  __u8 initiator_depth;
  __u8 remote_cm_response_timeout;
  __u8 flow_control;
  __u8 local_cm_response_timeout;
  __u8 retry_count;
  __u8 rnr_retry_count;
  __u8 max_cm_retries;
  __u8 srq;
  __u8 reserved[5];
};
struct ib_ucm_rep {
  __u64 uid;
  __u64 data;
  __u32 id;
  __u32 qpn;
  __u32 psn;
  __u8 len;
  __u8 responder_resources;
  __u8 initiator_depth;
  __u8 target_ack_delay;
  __u8 failover_accepted;
  __u8 flow_control;
  __u8 rnr_retry_count;
  __u8 srq;
  __u8 reserved[4];
};
struct ib_ucm_info {
  __u32 id;
  __u32 status;
  __u64 info;
  __u64 data;
  __u8 info_len;
  __u8 data_len;
  __u8 reserved[6];
};
struct ib_ucm_mra {
  __u64 data;
  __u32 id;
  __u8 len;
  __u8 timeout;
  __u8 reserved[2];
};
struct ib_ucm_lap {
  __u64 path;
  __u64 data;
  __u32 id;
  __u8 len;
  __u8 reserved[3];
};
struct ib_ucm_sidr_req {
  __u32 id;
  __u32 timeout;
  __be64 sid;
  __u64 data;
  __u64 path;
  __u16 reserved_pkey;
  __u8 len;
  __u8 max_cm_retries;
  __u8 reserved[4];
};
struct ib_ucm_sidr_rep {
  __u32 id;
  __u32 qpn;
  __u32 qkey;
  __u32 status;
  __u64 info;
  __u64 data;
  __u8 info_len;
  __u8 data_len;
  __u8 reserved[6];
};
struct ib_ucm_event_get {
  __u64 response;
  __u64 data;
  __u64 info;
  __u8 data_len;
  __u8 info_len;
  __u8 reserved[6];
};
struct ib_ucm_req_event_resp {
  struct ib_user_path_rec primary_path;
  struct ib_user_path_rec alternate_path;
  __be64 remote_ca_guid;
  __u32 remote_qkey;
  __u32 remote_qpn;
  __u32 qp_type;
  __u32 starting_psn;
  __u8 responder_resources;
  __u8 initiator_depth;
  __u8 local_cm_response_timeout;
  __u8 flow_control;
  __u8 remote_cm_response_timeout;
  __u8 retry_count;
  __u8 rnr_retry_count;
  __u8 srq;
  __u8 port;
  __u8 reserved[7];
};
struct ib_ucm_rep_event_resp {
  __be64 remote_ca_guid;
  __u32 remote_qkey;
  __u32 remote_qpn;
  __u32 starting_psn;
  __u8 responder_resources;
  __u8 initiator_depth;
  __u8 target_ack_delay;
  __u8 failover_accepted;
  __u8 flow_control;
  __u8 rnr_retry_count;
  __u8 srq;
  __u8 reserved[5];
};
struct ib_ucm_rej_event_resp {
  __u32 reason;
};
struct ib_ucm_mra_event_resp {
  __u8 timeout;
  __u8 reserved[3];
};
struct ib_ucm_lap_event_resp {
  struct ib_user_path_rec path;
};
struct ib_ucm_apr_event_resp {
  __u32 status;
};
struct ib_ucm_sidr_req_event_resp {
  __u16 pkey;
  __u8 port;
  __u8 reserved;
};
struct ib_ucm_sidr_rep_event_resp {
  __u32 status;
  __u32 qkey;
  __u32 qpn;
};
#define IB_UCM_PRES_DATA 0x01
#define IB_UCM_PRES_INFO 0x02
#define IB_UCM_PRES_PRIMARY 0x04
#define IB_UCM_PRES_ALTERNATE 0x08
struct ib_ucm_event_resp {
  __u64 uid;
  __u32 id;
  __u32 event;
  __u32 present;
  __u32 reserved;
  union {
    struct ib_ucm_req_event_resp req_resp;
    struct ib_ucm_rep_event_resp rep_resp;
    struct ib_ucm_rej_event_resp rej_resp;
    struct ib_ucm_mra_event_resp mra_resp;
    struct ib_ucm_lap_event_resp lap_resp;
    struct ib_ucm_apr_event_resp apr_resp;
    struct ib_ucm_sidr_req_event_resp sidr_req_resp;
    struct ib_ucm_sidr_rep_event_resp sidr_rep_resp;
    __u32 send_status;
  } u;
};
#endif

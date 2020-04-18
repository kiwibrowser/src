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
#ifndef _DVBCA_H_
#define _DVBCA_H_
typedef struct ca_slot_info {
  int num;
  int type;
#define CA_CI 1
#define CA_CI_LINK 2
#define CA_CI_PHYS 4
#define CA_DESCR 8
#define CA_SC 128
  unsigned int flags;
#define CA_CI_MODULE_PRESENT 1
#define CA_CI_MODULE_READY 2
} ca_slot_info_t;
typedef struct ca_descr_info {
  unsigned int num;
  unsigned int type;
#define CA_ECD 1
#define CA_NDS 2
#define CA_DSS 4
} ca_descr_info_t;
typedef struct ca_caps {
  unsigned int slot_num;
  unsigned int slot_type;
  unsigned int descr_num;
  unsigned int descr_type;
} ca_caps_t;
typedef struct ca_msg {
  unsigned int index;
  unsigned int type;
  unsigned int length;
  unsigned char msg[256];
} ca_msg_t;
typedef struct ca_descr {
  unsigned int index;
  unsigned int parity;
  unsigned char cw[8];
} ca_descr_t;
typedef struct ca_pid {
  unsigned int pid;
  int index;
} ca_pid_t;
#define CA_RESET _IO('o', 128)
#define CA_GET_CAP _IOR('o', 129, ca_caps_t)
#define CA_GET_SLOT_INFO _IOR('o', 130, ca_slot_info_t)
#define CA_GET_DESCR_INFO _IOR('o', 131, ca_descr_info_t)
#define CA_GET_MSG _IOR('o', 132, ca_msg_t)
#define CA_SEND_MSG _IOW('o', 133, ca_msg_t)
#define CA_SET_DESCR _IOW('o', 134, ca_descr_t)
#define CA_SET_PID _IOW('o', 135, ca_pid_t)
#endif

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
#ifndef _ASM_MSGBUF_H
#define _ASM_MSGBUF_H
struct msqid64_ds {
  struct ipc64_perm msg_perm;
  __kernel_time_t msg_stime;
#ifndef __mips64
  unsigned long __unused1;
#endif
  __kernel_time_t msg_rtime;
#ifndef __mips64
  unsigned long __unused2;
#endif
  __kernel_time_t msg_ctime;
#ifndef __mips64
  unsigned long __unused3;
#endif
  unsigned long msg_cbytes;
  unsigned long msg_qnum;
  unsigned long msg_qbytes;
  __kernel_pid_t msg_lspid;
  __kernel_pid_t msg_lrpid;
  unsigned long __unused4;
  unsigned long __unused5;
};
#endif

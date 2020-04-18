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
#ifndef _ASM_X86_SEMBUF_H
#define _ASM_X86_SEMBUF_H
struct semid64_ds {
  struct ipc64_perm sem_perm;
  __kernel_time_t sem_otime;
  __kernel_ulong_t __unused1;
  __kernel_time_t sem_ctime;
  __kernel_ulong_t __unused2;
  __kernel_ulong_t sem_nsems;
  __kernel_ulong_t __unused3;
  __kernel_ulong_t __unused4;
};
#endif

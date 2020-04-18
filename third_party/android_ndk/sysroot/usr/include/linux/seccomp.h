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
#ifndef _UAPI_LINUX_SECCOMP_H
#define _UAPI_LINUX_SECCOMP_H
#include <linux/compiler.h>
#include <linux/types.h>
#define SECCOMP_MODE_DISABLED 0
#define SECCOMP_MODE_STRICT 1
#define SECCOMP_MODE_FILTER 2
#define SECCOMP_SET_MODE_STRICT 0
#define SECCOMP_SET_MODE_FILTER 1
#define SECCOMP_FILTER_FLAG_TSYNC 1
#define SECCOMP_RET_KILL 0x00000000U
#define SECCOMP_RET_TRAP 0x00030000U
#define SECCOMP_RET_ERRNO 0x00050000U
#define SECCOMP_RET_TRACE 0x7ff00000U
#define SECCOMP_RET_ALLOW 0x7fff0000U
#define SECCOMP_RET_ACTION 0x7fff0000U
#define SECCOMP_RET_DATA 0x0000ffffU
struct seccomp_data {
  int nr;
  __u32 arch;
  __u64 instruction_pointer;
  __u64 args[6];
};
#endif

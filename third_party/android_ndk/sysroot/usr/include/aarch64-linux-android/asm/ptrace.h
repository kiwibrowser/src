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
#ifndef _UAPI__ASM_PTRACE_H
#define _UAPI__ASM_PTRACE_H
#include <linux/types.h>
#include <asm/hwcap.h>
#define PSR_MODE_EL0t 0x00000000
#define PSR_MODE_EL1t 0x00000004
#define PSR_MODE_EL1h 0x00000005
#define PSR_MODE_EL2t 0x00000008
#define PSR_MODE_EL2h 0x00000009
#define PSR_MODE_EL3t 0x0000000c
#define PSR_MODE_EL3h 0x0000000d
#define PSR_MODE_MASK 0x0000000f
#define PSR_MODE32_BIT 0x00000010
#define PSR_F_BIT 0x00000040
#define PSR_I_BIT 0x00000080
#define PSR_A_BIT 0x00000100
#define PSR_D_BIT 0x00000200
#define PSR_PAN_BIT 0x00400000
#define PSR_UAO_BIT 0x00800000
#define PSR_Q_BIT 0x08000000
#define PSR_V_BIT 0x10000000
#define PSR_C_BIT 0x20000000
#define PSR_Z_BIT 0x40000000
#define PSR_N_BIT 0x80000000
#define PSR_f 0xff000000
#define PSR_s 0x00ff0000
#define PSR_x 0x0000ff00
#define PSR_c 0x000000ff
#ifndef __ASSEMBLY__
struct user_pt_regs {
  __u64 regs[31];
  __u64 sp;
  __u64 pc;
  __u64 pstate;
};
struct user_fpsimd_state {
  __uint128_t vregs[32];
  __u32 fpsr;
  __u32 fpcr;
  __u32 __reserved[2];
};
struct user_hwdebug_state {
  __u32 dbg_info;
  __u32 pad;
  struct {
    __u64 addr;
    __u32 ctrl;
    __u32 pad;
  } dbg_regs[16];
};
#endif
#endif

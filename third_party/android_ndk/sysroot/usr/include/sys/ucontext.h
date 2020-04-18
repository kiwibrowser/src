/*
 * Copyright (C) 2014 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_UCONTEXT_H_
#define _SYS_UCONTEXT_H_

#include <sys/cdefs.h>

#include <signal.h>
#include <sys/user.h>

__BEGIN_DECLS

#if defined(__arm__)

enum {
  REG_R0 = 0,
#define REG_R0 REG_R0
  REG_R1,
#define REG_R1 REG_R1
  REG_R2,
#define REG_R2 REG_R2
  REG_R3,
#define REG_R3 REG_R3
  REG_R4,
#define REG_R4 REG_R4
  REG_R5,
#define REG_R5 REG_R5
  REG_R6,
#define REG_R6 REG_R6
  REG_R7,
#define REG_R7 REG_R7
  REG_R8,
#define REG_R8 REG_R8
  REG_R9,
#define REG_R9 REG_R9
  REG_R10,
#define REG_R10 REG_R10
  REG_R11,
#define REG_R11 REG_R11
  REG_R12,
#define REG_R12 REG_R12
  REG_R13,
#define REG_R13 REG_R13
  REG_R14,
#define REG_R14 REG_R14
  REG_R15,
#define REG_R15 REG_R15
};

#define NGREG 18 /* Like glibc. */

typedef int greg_t;
typedef greg_t gregset_t[NGREG];
typedef struct user_fpregs fpregset_t;

#include <asm/sigcontext.h>
typedef struct sigcontext mcontext_t;

typedef struct ucontext {
  unsigned long uc_flags;
  struct ucontext* uc_link;
  stack_t uc_stack;
  mcontext_t uc_mcontext;
  sigset_t uc_sigmask;
  /* Android has a wrong (smaller) sigset_t on ARM. */
  uint32_t __padding_rt_sigset;
  /* The kernel adds extra padding after uc_sigmask to match glibc sigset_t on ARM. */
  char __padding[120];
  unsigned long uc_regspace[128] __attribute__((__aligned__(8)));
} ucontext_t;

#elif defined(__aarch64__)

#define NGREG 34 /* x0..x30 + sp + pc + pstate */
typedef unsigned long greg_t;
typedef greg_t gregset_t[NGREG];
typedef struct user_fpsimd_struct fpregset_t;

#include <asm/sigcontext.h>
typedef struct sigcontext mcontext_t;

typedef struct ucontext {
  unsigned long uc_flags;
  struct ucontext *uc_link;
  stack_t uc_stack;
  sigset_t uc_sigmask;
  /* The kernel adds extra padding after uc_sigmask to match glibc sigset_t on ARM64. */
  char __padding[128 - sizeof(sigset_t)];
  mcontext_t uc_mcontext;
} ucontext_t;

#elif defined(__i386__)

enum {
  REG_GS = 0,
#define REG_GS REG_GS
  REG_FS,
#define REG_FS REG_FS
  REG_ES,
#define REG_ES REG_ES
  REG_DS,
#define REG_DS REG_DS
  REG_EDI,
#define REG_EDI REG_EDI
  REG_ESI,
#define REG_ESI REG_ESI
  REG_EBP,
#define REG_EBP REG_EBP
  REG_ESP,
#define REG_ESP REG_ESP
  REG_EBX,
#define REG_EBX REG_EBX
  REG_EDX,
#define REG_EDX REG_EDX
  REG_ECX,
#define REG_ECX REG_ECX
  REG_EAX,
#define REG_EAX REG_EAX
  REG_TRAPNO,
#define REG_TRAPNO REG_TRAPNO
  REG_ERR,
#define REG_ERR REG_ERR
  REG_EIP,
#define REG_EIP REG_EIP
  REG_CS,
#define REG_CS REG_CS
  REG_EFL,
#define REG_EFL REG_EFL
  REG_UESP,
#define REG_UESP REG_UESP
  REG_SS,
#define REG_SS REG_SS
  NGREG
#define NGREG NGREG
};

typedef int greg_t;
typedef greg_t gregset_t[NGREG];

struct _libc_fpreg {
  unsigned short significand[4];
  unsigned short exponent;
};

struct _libc_fpstate {
  unsigned long cw;
  unsigned long sw;
  unsigned long tag;
  unsigned long ipoff;
  unsigned long cssel;
  unsigned long dataoff;
  unsigned long datasel;
  struct _libc_fpreg _st[8];
  unsigned long status;
};

typedef struct _libc_fpstate* fpregset_t;

typedef struct {
  gregset_t gregs;
  fpregset_t fpregs;
  unsigned long oldmask;
  unsigned long cr2;
} mcontext_t;

typedef struct ucontext {
  unsigned long uc_flags;
  struct ucontext* uc_link;
  stack_t uc_stack;
  mcontext_t uc_mcontext;
  sigset_t uc_sigmask;
  /* Android has a wrong (smaller) sigset_t on x86. */
  uint32_t __padding_rt_sigset;
  struct _libc_fpstate __fpregs_mem;
} ucontext_t;

#elif defined(__mips__)

/* glibc doesn't have names for MIPS registers. */

#define NGREG 32
#define NFPREG 32

typedef unsigned long long greg_t;
typedef greg_t gregset_t[NGREG];

typedef struct fpregset {
  union {
    double fp_dregs[NFPREG];
    struct {
      float _fp_fregs;
      unsigned _fp_pad;
    } fp_fregs[NFPREG];
  } fp_r;
} fpregset_t;

#ifdef __LP64__
typedef struct {
  gregset_t gregs;
  fpregset_t fpregs;
  greg_t mdhi;
  greg_t hi1;
  greg_t hi2;
  greg_t hi3;
  greg_t mdlo;
  greg_t lo1;
  greg_t lo2;
  greg_t lo3;
  greg_t pc;
  uint32_t fpc_csr;
  uint32_t used_math;
  uint32_t dsp;
  uint32_t reserved;
} mcontext_t;
#else
typedef struct {
  unsigned regmask;
  unsigned status;
  greg_t pc;
  gregset_t gregs;
  fpregset_t fpregs;
  unsigned fp_owned;
  unsigned fpc_csr;
  unsigned fpc_eir;
  unsigned used_math;
  unsigned dsp;
  greg_t mdhi;
  greg_t mdlo;
  unsigned long hi1;
  unsigned long lo1;
  unsigned long hi2;
  unsigned long lo2;
  unsigned long hi3;
  unsigned long lo3;
} mcontext_t;
#endif

typedef struct ucontext {
  unsigned long uc_flags;
  struct ucontext* uc_link;
  stack_t uc_stack;
  mcontext_t uc_mcontext;
  sigset_t uc_sigmask;
} ucontext_t;

#elif defined(__x86_64__)

enum {
  REG_R8 = 0,
#define REG_R8 REG_R8
  REG_R9,
#define REG_R9 REG_R9
  REG_R10,
#define REG_R10 REG_R10
  REG_R11,
#define REG_R11 REG_R11
  REG_R12,
#define REG_R12 REG_R12
  REG_R13,
#define REG_R13 REG_R13
  REG_R14,
#define REG_R14 REG_R14
  REG_R15,
#define REG_R15 REG_R15
  REG_RDI,
#define REG_RDI REG_RDI
  REG_RSI,
#define REG_RSI REG_RSI
  REG_RBP,
#define REG_RBP REG_RBP
  REG_RBX,
#define REG_RBX REG_RBX
  REG_RDX,
#define REG_RDX REG_RDX
  REG_RAX,
#define REG_RAX REG_RAX
  REG_RCX,
#define REG_RCX REG_RCX
  REG_RSP,
#define REG_RSP REG_RSP
  REG_RIP,
#define REG_RIP REG_RIP
  REG_EFL,
#define REG_EFL REG_EFL
  REG_CSGSFS,
#define REG_CSGSFS REG_CSGSFS
  REG_ERR,
#define REG_ERR REG_ERR
  REG_TRAPNO,
#define REG_TRAPNO REG_TRAPNO
  REG_OLDMASK,
#define REG_OLDMASK REG_OLDMASK
  REG_CR2,
#define REG_CR2 REG_CR2
  NGREG
#define NGREG NGREG
};

typedef long greg_t;
typedef greg_t gregset_t[NGREG];

struct _libc_fpxreg {
  unsigned short significand[4];
  unsigned short exponent;
  unsigned short padding[3];
};

struct _libc_xmmreg {
  uint32_t element[4];
};

struct _libc_fpstate {
  uint16_t cwd;
  uint16_t swd;
  uint16_t ftw;
  uint16_t fop;
  uint64_t rip;
  uint64_t rdp;
  uint32_t mxcsr;
  uint32_t mxcr_mask;
  struct _libc_fpxreg _st[8];
  struct _libc_xmmreg _xmm[16];
  uint32_t padding[24];
};

typedef struct _libc_fpstate* fpregset_t;

typedef struct {
  gregset_t gregs;
  fpregset_t fpregs;
  unsigned long __reserved1[8];
} mcontext_t;

typedef struct ucontext {
  unsigned long uc_flags;
  struct ucontext* uc_link;
  stack_t uc_stack;
  mcontext_t uc_mcontext;
  sigset_t uc_sigmask;
  struct _libc_fpstate __fpregs_mem;
} ucontext_t;

#endif

__END_DECLS

#endif /* _SYS_UCONTEXT_H_ */

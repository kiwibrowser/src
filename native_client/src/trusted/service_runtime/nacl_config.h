/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 *
 * NOTE: This header is ALSO included by assembler files and hence
 *       must not include any C code
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_CONFIG_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_CONFIG_H_

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_asm.h"

/* maximum number of elf program headers allowed. */
#define NACL_MAX_PROGRAM_HEADERS  128

/*
 * NACL_BLOCK_SHIFT is defined per-architecture, below.
 *
 * This value must be consistent with what the compiler generates.
 * In current toolchains, this is hard-coded and not configurable.
 */
#define NACL_INSTR_BLOCK_SHIFT        (NACL_BLOCK_SHIFT)
#define NACL_INSTR_BLOCK_SIZE         (1 << NACL_INSTR_BLOCK_SHIFT)

/* this must be a multiple of the system page size */
#define NACL_PAGESHIFT                12
#define NACL_PAGESIZE                 (1U << NACL_PAGESHIFT)

#define NACL_MAP_PAGESHIFT            16
#define NACL_MAP_PAGESIZE             (1U << NACL_MAP_PAGESHIFT)

#if NACL_MAP_PAGESHIFT < NACL_PAGESHIFT
# error "NACL_MAP_PAGESHIFT smaller than NACL_PAGESHIFT"
#endif

/* NACL_MAP_PAGESIFT >= NACL_PAGESHIFT must hold */
#define NACL_PAGES_PER_MAP            (1 << (NACL_MAP_PAGESHIFT-NACL_PAGESHIFT))

#define NACL_MEMORY_ALLOC_RETRY_MAX   256 /* see win/sel_memory.c */

/*
 * NACL_KERN_STACK_SIZE: The size of the secure stack allocated for
 * use while a NaCl thread is in kernel mode, i.e., during
 * startup/exit and during system call processing.
 *
 * This must be greater than 64K for address space squatting, but
 * that uses up too much memory and decreases the maximum number of
 * threads that the system can spawn.  Instead, we grab the vm lock
 * when spawning threads.
 */
#define NACL_KERN_STACK_SIZE          (64 << 10)

/*
 * NACL_CONFIG_PATH_MAX: What is the maximum file path allowed in the
 * NaClSysOpen syscall?  This is on the kernel stack for validation
 * during the processing of the syscall, so beware running into
 * NACL_KERN_STACK_SIZE above.
 */
#define NACL_CONFIG_PATH_MAX          1024

/*
 * newfd value for dup2 must be below this value.
 */
#define NACL_MAX_FD                   4096

/*
 * Macro for the start address of the trampolines.
 */
/*
 * The first 64KB (16 pages) are inaccessible.  On x86, this is to prevent
 * addr16/data16 attacks.
 */
#define NACL_SYSCALL_START_ADDR       (16 << NACL_PAGESHIFT)
/* Macro for the start address of a specific trampoline.  */
#define NACL_SYSCALL_ADDR(syscall_number) \
    (NACL_SYSCALL_START_ADDR + (syscall_number << NACL_SYSCALL_BLOCK_SHIFT))

/*
 * Syscall trampoline code have a block size that may differ from the
 * alignment restrictions of the executable.  The ELF executable's
 * alignment restriction (16 or 32) defines what are potential entry
 * points, so the trampoline region must still respect that.  We
 * prefill the trampoline region with HLT, so non-syscall entry points
 * will not cause problems as long as our trampoline instruction
 * sequence never grows beyond 16 bytes, so the "odd" potential entry
 * points for a 16-byte aligned ELF will not be able to jump into the
 * middle of the trampoline code.
 */
#define NACL_SYSCALL_BLOCK_SHIFT      5
#define NACL_SYSCALL_BLOCK_SIZE       (1 << NACL_SYSCALL_BLOCK_SHIFT)

/*
 * the extra space for the trampoline syscall code and the thread
 * contexts must be a multiple of the page size.
 *
 * The address space begins with a 64KB region that is inaccessible to
 * handle NULL pointers and also to reinforce protection agasint abuse of
 * addr16/data16 prefixes.
 * NACL_TRAMPOLINE_START gives the address of the first trampoline.
 * NACL_TRAMPOLINE_END gives the address of the first byte after the
 * trampolines.
 */
#define NACL_NULL_REGION_SHIFT  16
#define NACL_TRAMPOLINE_START   (1 << NACL_NULL_REGION_SHIFT)
#define NACL_TRAMPOLINE_SHIFT   16
#define NACL_TRAMPOLINE_SIZE    (1 << NACL_TRAMPOLINE_SHIFT)
#define NACL_TRAMPOLINE_END     (NACL_TRAMPOLINE_START + NACL_TRAMPOLINE_SIZE)

/*
 * Extra required space at the end of static text (and dynamic text,
 * if any).  The intent is to stop a thread from "walking off the end"
 * of a text region into another.  Four bytes suffices for all
 * currently supported architectures (halt is one byte on x86-32 and
 * x86-64, and 4 bytes on ARM), but for x86 we want to provide
 * paranoia-in-depth: if the NX bit (x86-64, arm) or the %cs segment
 * protection (x86-32) fails on weird borderline cases and the
 * validator also fails to detect a weird instruction sequence, we may
 * have the following situation: a partial multi-byte instruction is
 * placed on the end of the last page, and by "absorbing" the trailing
 * HALT instructions (esp if there are none) as immediate data (or
 * something similar), cause the PC to go outside of the untrusted
 * code space, possibly into data memory.  By requiring 32 bytes of
 * space to fill with HALT instructions, we (attempt to) ensure that
 * such op-code absorption cannot happen, and at least one of these
 * HALTs will cause the untrusted thread to abort, and take down the
 * whole NaCl app.
 */
#define NACL_HALT_SLED_SIZE     32

/*
 * NACL_FAKE_INODE_NUM is used throughout as inode number returned in
 * stat/fstat/getdents system calls, unless NaClAclBypassChecks is
 * set in which case the real inode numbers are reported.
 * Exposing inode numbers are a miniscule information leak; more importantly,
 * it is yet another platform difference since none of the standard
 * Windows filesystems have inode numbers.
 */
#if !defined(NACL_FAKE_INODE_NUM) /* allow alternate value */
# define NACL_FAKE_INODE_NUM     0x6c43614e
#endif

/*
 * Block sizes, op-codes that are used in NaCl, and trampoline related
 * constants that simplify making the C code architecture independent.
 */
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86

# define NACL_BLOCK_SHIFT         (5)

# define NACL_NOOP_OPCODE    0x90
# define NACL_HALT_OPCODE    0xf4
# define NACL_HALT_LEN       1           /* length of halt instruction */
# define NACL_HALT_WORD      0xf4f4f4f4U

# define NACL_X86_TRAP_FLAG      (1 << 8)
# define NACL_X86_DIRECTION_FLAG (1 << 10)

# if NACL_BUILD_SUBARCH == 32
#  define NACL_ELF_E_MACHINE      EM_386
/*
 * The untrusted stack looks like this on x86-32:
 *   esp-0x0: syscall args pushed by untrusted code before calling trampoline
 *   esp-0x4: 4 byte return address pushed by untrusted code's call
 *   esp-0xc: 8 bytes pushed by the trampoline's lcall instruction
 */
#  define NACL_TRAMPRET_FIX       (-0xc)
#  define NACL_USERRET_FIX        (-0x4)
#  define NACL_SYSARGS_FIX        (0)
/*
 * System V Application Binary Interface, Intel386 Architcture
 * Processor Supplement, section 3-10, says stack alignment is
 * 4-bytes, but gcc-generated code can require 16-byte alignment
 * (depending on compiler flags in force) for SSE instructions, so we
 * must do so here as well.
 */
#  define NACL_STACK_ALIGN_MASK   (0xf)
#  define NACL_STACK_ARGS_SIZE    (0)
#  define NACL_STACK_GETS_ARG     (1)
#  define NACL_STACK_PAD_BELOW_ALIGN (4)
#  define NACL_STACK_RED_ZONE     (0)

# elif NACL_BUILD_SUBARCH == 64
#  define NACL_ELF_E_MACHINE      EM_X86_64
/*
 * The untrusted stack looks like this on x86-64:
 *   rsp-0x08: 8 byte return address pushed by untrusted code's call
 *   rsp-0x10: 4 byte pseudo return address saved by the trampoline
 *   rsp-0x28: 0x18 bytes of syscall arguments saved by NaClSyscallSeg
 */
#  define NACL_TRAMPRET_FIX       (-0x10)
#  define NACL_USERRET_FIX        (-0x8)
#  define NACL_SYSARGS_FIX        (-0x28)
/*
 * System V Application Binary Interface, AMD64 Architecture Processor
 * Supplement, at http://www.x86-64.org/documentation/abi.pdf, section
 * 3.2.2 discusses stack alignment.
 */
#  define NACL_STACK_ALIGN_MASK   (0xf)
#  define NACL_STACK_ARGS_SIZE    (0)
#  define NACL_STACK_GETS_ARG     (0)
#  define NACL_STACK_PAD_BELOW_ALIGN (8)
#  define NACL_STACK_RED_ZONE     (128)
# else /* NACL_BUILD_SUBARCH */
#  error Unknown platform!
# endif /* NACL_BUILD_SUBARCH */

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
# include "native_client/src/include/arm_sandbox.h"

# define NACL_ELF_E_MACHINE       EM_ARM
# define NACL_BLOCK_SHIFT         (4)

# define NACL_NOOP_OPCODE         0xe1a00000  /* mov r0, r0 */
# define NACL_HALT_OPCODE         NACL_INSTR_ARM_HALT_FILL
# define NACL_HALT_LEN            4           /* length of halt instruction */
# define NACL_HALT_WORD           NACL_HALT_OPCODE

/* 16-byte bundles, 1G address space */
# define NACL_CONTROL_FLOW_MASK      0xC000000F

# define NACL_DATA_FLOW_MASK      0xC0000000

/*
 * The untrusted stack looks like this on ARM:
 *   sp-0x00: 0-8 bytes for 0-2 syscall arguments pushed by untrusted code
 *   sp-0x10: 16 bytes for 4 syscall arguments saved by the trampoline
 *   sp-0x14: 4 byte return address (value of lr) saved by the trampoline
 *   sp-0x18: 4 byte return address (value of lr) saved by NaClSyscallSeg
 */
# define NACL_TRAMPRET_FIX        (-0x18)
# define NACL_USERRET_FIX         (-0x14)
# define NACL_SYSARGS_FIX         (-0x10)
/*
 * See ARM Procedure Call Standard, ARM IHI 0042D, section 5.2.1.2.
 * http://infocenter.arm.com/help/topic/com.arm.doc.ihi0042a/IHI0042A_aapcs.pdf
 * -- the "public" stack alignment is required to be 8 bytes.
 * While ARM vector loads and stores work with misaligned addresses, there can
 * be performance penalties on some microarchitectures.  To improve the
 * performance of vector instructions, we increase this to 16.
 */
# define NACL_STACK_ALIGN_MASK    (0xf)
# define NACL_STACK_ARGS_SIZE     (0)
# define NACL_STACK_GETS_ARG      (0)
# define NACL_STACK_PAD_BELOW_ALIGN (0)
# define NACL_STACK_RED_ZONE      (0)

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips

# undef  NACL_KERN_STACK_SIZE        // Mips needs 128k pthread stack size
# define NACL_KERN_STACK_SIZE        (128 << 10)

# define NACL_ELF_E_MACHINE          EM_MIPS
# define NACL_BLOCK_SHIFT            4

# define NACL_NOOP_OPCODE        0x00000000  /* nop */
# define NACL_HALT_OPCODE        0x0000000D  /* break */
# define NACL_HALT_LEN           4           /* length of halt instruction */
# define NACL_HALT_WORD          NACL_HALT_OPCODE

/* 16-byte bundles, 256MB code segment*/
# define NACL_CONTROL_FLOW_MASK     0x0FFFFFF0
# define NACL_DATA_FLOW_MASK        0x3FFFFFFF
/*
 * The untrusted stack looks like this on MIPS:
 *   $sp+0x10: 0-8 bytes for 0-2 syscall arguments saved by untrusted code
 *   $sp+0x00: 16 bytes for 4 syscall arguments saved by NaClSyscallSeg
 *   $sp-0x04: 4 byte user return address saved by NaClSyscallSeg
 *   $sp-0x08: 4 byte trampoline address saved by NaClSyscallSeg
 */
# define NACL_TRAMPRET_FIX          (-0x8)
# define NACL_USERRET_FIX           (-0x4)
# define NACL_SYSARGS_FIX           (0)
# define NACL_STACK_ALIGN_MASK      (0x7)
/*
 * The MIPS o32 ABI requires callers to reserve 16 bytes above the stack
 * pointer, which the callee can spill arguments to.
 */
# define NACL_STACK_ARGS_SIZE       (0x10)
# define NACL_STACK_GETS_ARG        (0)
# define NACL_STACK_PAD_BELOW_ALIGN (0)
# define NACL_STACK_RED_ZONE        (0)
/* 16 byte bundles */

#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_pnacl

/* This is a PNaCl build, so we define no arch-dependent stuff */

#else /* NACL_ARCH(NACL_BUILD_ARCH) */

# error Unknown platform!

#endif /* NACL_ARCH(NACL_BUILD_ARCH) */

#ifdef __ASSEMBLER__
# if NACL_WINDOWS
#  define NACL_RODATA           .section .rdata, "dr"
# elif NACL_OSX
#  define NACL_RODATA           .section __TEXT, __const
# else
#  define NACL_RODATA           .section .rodata, "a"
# endif
#endif

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_CONFIG_H_ */

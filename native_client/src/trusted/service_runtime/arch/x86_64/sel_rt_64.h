/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Secure Runtime
 */

#ifndef __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_64_SEL_RT_64_H__
#define __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_64_SEL_RT_64_H__ 1

/* This file can be #included from assembly to get the #defines. */
#if !defined(__ASSEMBLER__)

#include <stddef.h>

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"

EXTERN_C_BEGIN

void NaClDoFnstcw(uint16_t *fcw);
void NaClDoFxsave(void *fxsave);
void NaClDoFxrstor(void *fxsave);

void NaClSyscallSeg(void);
void NaClSyscallSegRegsSaved(void);

typedef uint64_t  nacl_reg_t;  /* general purpose register type */

#define NACL_PRIdNACL_REG NACL_PRId64
#define NACL_PRIiNACL_REG NACL_PRIi64
#define NACL_PRIoNACL_REG NACL_PRIo64
#define NACL_PRIuNACL_REG NACL_PRIu64
#define NACL_PRIxNACL_REG NACL_PRIx64
#define NACL_PRIXNACL_REG NACL_PRIX64

/*
 * The layout of NaClThreadContext must be kept in sync with the
 * #defines below.
 */
struct NaClThreadContext {
  /*
   * nacl64-gdb hard-codes the offsets of rbx, rbp, r12, r13 and r14,
   * so these fields should not be removed until nacl64-gdb is fully
   * replaced by debugging via the GDB debug stub.
   */
  nacl_reg_t  rax,  rbx,  rcx,  rdx,  rbp,  rsi,  rdi,  rsp;
  /*          0x0,  0x8, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38 */
  nacl_reg_t  r8,     r9,  r10,  r11,  r12,  r13,  r14,  r15;
  /*          0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78 */
  nacl_reg_t  prog_ctr;  /* rip */
  /*          0x80 */

  nacl_reg_t  new_prog_ctr;
  /*          0x88 */
  nacl_reg_t  sysret;
  /*          0x90 */
  uint32_t    mxcsr;
  /*          0x98 */
  uint32_t    sys_mxcsr;
  /*          0x9c */
  uint32_t    tls_idx;
  /*          0xa0 */
  uint16_t    fcw;
  /*          0xa4 */
  uint16_t    sys_fcw;
  /*          0xa6 */
  uint64_t    trusted_stack_ptr;
  /*          0xa8 */
  uint32_t    tls_value1;
  /*          0xb0 */
  uint32_t    tls_value2;
  /*          0xb4 */
};

static INLINE uintptr_t NaClGetThreadCtxSp(struct NaClThreadContext *th_ctx) {
  return th_ctx->rsp;
}

#endif /* !defined(__ASSEMBLER__) */

#define NACL_THREAD_CONTEXT_OFFSET_RAX           0x00
#define NACL_THREAD_CONTEXT_OFFSET_RBX           0x08
#define NACL_THREAD_CONTEXT_OFFSET_RCX           0x10
#define NACL_THREAD_CONTEXT_OFFSET_RDX           0x18
#define NACL_THREAD_CONTEXT_OFFSET_RBP           0x20
#define NACL_THREAD_CONTEXT_OFFSET_RSI           0x28
#define NACL_THREAD_CONTEXT_OFFSET_RDI           0x30
#define NACL_THREAD_CONTEXT_OFFSET_RSP           0x38
#define NACL_THREAD_CONTEXT_OFFSET_R8            0x40
#define NACL_THREAD_CONTEXT_OFFSET_R9            0x48
#define NACL_THREAD_CONTEXT_OFFSET_R10           0x50
#define NACL_THREAD_CONTEXT_OFFSET_R11           0x58
#define NACL_THREAD_CONTEXT_OFFSET_R12           0x60
#define NACL_THREAD_CONTEXT_OFFSET_R13           0x68
#define NACL_THREAD_CONTEXT_OFFSET_R14           0x70
#define NACL_THREAD_CONTEXT_OFFSET_R15           0x78
#define NACL_THREAD_CONTEXT_OFFSET_PROG_CTR      0x80
#define NACL_THREAD_CONTEXT_OFFSET_NEW_PROG_CTR  0x88
#define NACL_THREAD_CONTEXT_OFFSET_SYSRET        0x90
#define NACL_THREAD_CONTEXT_OFFSET_MXCSR         0x98
#define NACL_THREAD_CONTEXT_OFFSET_SYS_MXCSR     0x9c
#define NACL_THREAD_CONTEXT_OFFSET_TLS_IDX       0xa0
#define NACL_THREAD_CONTEXT_OFFSET_FCW           0xa4
#define NACL_THREAD_CONTEXT_OFFSET_SYS_FCW       0xa6
#define NACL_THREAD_CONTEXT_OFFSET_TRUSTED_STACK_PTR 0xa8
#define NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE1    0xb0
#define NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE2    0xb4

#if !defined(__ASSEMBLER__)

static INLINE void NaClThreadContextOffsetCheck(void) {
  /*
   * We use 'offset' to check that every field of NaClThreadContext is
   * verified below.  The fields must be listed below in order.
   */
  int offset = 0;

#define NACL_CHECK_FIELD(offset_name, field) \
    NACL_COMPILE_TIME_ASSERT(offset_name == \
                             offsetof(struct NaClThreadContext, field)); \
    CHECK(offset == offset_name); \
    offset += sizeof(((struct NaClThreadContext *) NULL)->field);

  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_RAX, rax);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_RBX, rbx);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_RCX, rcx);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_RDX, rdx);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_RBP, rbp);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_RSI, rsi);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_RDI, rdi);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_RSP, rsp);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_R8, r8);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_R9, r9);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_R10, r10);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_R11, r11);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_R12, r12);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_R13, r13);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_R14, r14);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_R15, r15);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_PROG_CTR, prog_ctr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_NEW_PROG_CTR, new_prog_ctr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_SYSRET, sysret);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_MXCSR, mxcsr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_SYS_MXCSR, sys_mxcsr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TLS_IDX, tls_idx);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_FCW, fcw);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_SYS_FCW, sys_fcw);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TRUSTED_STACK_PTR,
                   trusted_stack_ptr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE1, tls_value1);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE2, tls_value2);
  CHECK(offset == sizeof(struct NaClThreadContext));

#undef NACL_CHECK_FIELD
}

EXTERN_C_END

#endif

/*
 * Argument passing convention in AMD64, from
 *
 *   http://www.x86-64.org/documentation/abi.pdf
 *
 * for system call parameters, are as follows.  All syscall arguments
 * are of INTEGER type (section 3.2.3).  They are assigned, from
 * left-to-right, to the registers
 *
 *   rdi, rsi, rdx, rcx, r8, r9
 *
 * and any additional arguments are passed on the stack, pushed onto
 * the stack in right-to-left order.  Note that this means that the
 * syscall with the maximum number of arguments, mmap, passes all its
 * arguments in registers.
 *
 * Argument passing convention for Microsoft, from wikipedia, is
 * different.  The first four arguments go in
 *
 *   rcx, rdx, r8, r9
 *
 * respectively, with the caller responsible for allocating 32 bytes
 * of "shadow space" for the first four arguments, an additional
 * arguments are on the stack.  Presumably this is to make stdargs
 * easier to implement: the callee can always write those four
 * registers to 8(%rsp), ..., 24(%rsp) resp (%rsp value assumed to be
 * at fn entry/start of prolog, before push %rbp), and then use the
 * effective address of 8(%rsp) as a pointer to an in-memory argument
 * list.  However, because this is always done, presumably called code
 * might treat this space as if it's part of the red zone, and it
 * would be an error to not allocate this stack space, even if the
 * called function is declared to take fewer than 4 arguments.
 *
 * Caller/callee saved
 *
 * - AMD64:
 *   - caller saved: rax, rcx, rdx, rdi, rsi, r8, r9, r10, r11
 *   - callee saved: rbx, rbp, r12, r13, r14, r15
 *
 * - Microsoft:
 *   - caller saved: rax, rcx, rdx, r8, r9, r10, r11
 *   - callee saved: rbx, rbp, rdi, rsi, r12, r13, r14, r15
 *
 * A conservative approach might be to follow microsoft and save more
 * registers, but the presence of shadow space will make assembly code
 * incompatible anyway, assembly code that calls must allocate shadow
 * space, but then in-memory arguments will be in the wrong location
 * wrt %rsp.
 */

#endif /* __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_64_SEL_RT_64_H__ */

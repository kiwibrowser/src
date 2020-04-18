/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Secure Runtime
 */

#ifndef __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_ARM_SEL_RT_H__
#define __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_ARM_SEL_RT_H__ 1

/* This file can be #included from assembly to get the #defines. */
#if !defined(__ASSEMBLER__)

#include <stddef.h>

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

uint32_t NaClGetStackPtr(void);

typedef uint32_t nacl_reg_t;

#define NACL_PRIdNACL_REG NACL_PRId32
#define NACL_PRIiNACL_REG NACL_PRIi32
#define NACL_PRIoNACL_REG NACL_PRIo32
#define NACL_PRIuNACL_REG NACL_PRIu32
#define NACL_PRIxNACL_REG NACL_PRIx32
#define NACL_PRIXNACL_REG NACL_PRIX32

struct NaClThreadContext {
  /*
   * r4 through to fp correspond to NACL_CALLEE_SAVE_LIST, and the assembly
   * code expects them to appear at the start of the struct.
   */
  nacl_reg_t  r4, r5, r6, r7, r8, r9, r10, fp, stack_ptr, prog_ctr;
  /*           0   4   8   c  10  14   18  1c         20        24 */
  /*
   * sys_ret and new_prog_ctr are not a part of the thread's register
   * set, but are needed by NaClSwitch.  By including them here, the
   * two use the same interface.
   */
  uint32_t  sysret;
  /*            28 */
  uint32_t  new_prog_ctr;
  /*            2c */
  uint32_t  trusted_stack_ptr;
  /*            30 */
  uint32_t  tls_idx;
  /*            34 */
  uint32_t  fpscr;
  /*            38 */
  uint32_t  sys_fpscr;
  /*            3c */
  uint32_t  tls_value1;
  /*            40 */
  uint32_t  tls_value2;
  /*            44 */
  uint32_t  syscall_routine;  /* Address of NaClSyscallSeg routine */
  /*            48 */
  uint32_t  guard_token;
  /*            4c */
};

static INLINE uintptr_t NaClGetThreadCtxSp(struct NaClThreadContext *th_ctx) {
  return th_ctx->stack_ptr;
}

NORETURN void NaClStartSwitch(struct NaClThreadContext *);

#endif /* !defined(__ASSEMBLER__) */

/*
 * List of registers at the start of NaClThreadContext, for use with the
 * instructions LDM and STM.
 *
 * Note that we omit "sp" from this list and save/restore it separately,
 * because using "sp" with LDM/STM is considered deprecated (see
 * https://crbug.com/564044).
 */
#define NACL_CALLEE_SAVE_LIST {r4, r5, r6, r7, r8, r9, r10, fp}

/*
 * Given an offset for a field in NaClThreadContext, this returns the
 * field's offset from r9, given that r9 points to tls_value1.
 */
#define NACL_R9_OFFSET(offset) \
    ((offset) - NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE1)

#define NACL_THREAD_CONTEXT_OFFSET_STACK_PTR 0x20
#define NACL_THREAD_CONTEXT_OFFSET_SYSRET 0x28
#define NACL_THREAD_CONTEXT_OFFSET_TRUSTED_STACK_PTR 0x30
#define NACL_THREAD_CONTEXT_OFFSET_TLS_IDX 0x34
#define NACL_THREAD_CONTEXT_OFFSET_FPSCR 0x38
#define NACL_THREAD_CONTEXT_OFFSET_SYS_FPSCR 0x3c
#define NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE1 0x40
#define NACL_THREAD_CONTEXT_OFFSET_SYSCALL_ROUTINE 0x48
#define NACL_THREAD_CONTEXT_OFFSET_GUARD_TOKEN 0x4c

#if !defined(__ASSEMBLER__)

/*
 * This function exists as a function only because compile-time
 * assertions need to be inside a function.  This function does not
 * need to be called for the assertions to be checked.
 */
static INLINE void NaClThreadContextOffsetCheck(void) {
#define NACL_CHECK_FIELD(offset_name, field) \
    NACL_COMPILE_TIME_ASSERT(offset_name == \
                             offsetof(struct NaClThreadContext, field));

  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_STACK_PTR, stack_ptr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_SYSRET, sysret);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TRUSTED_STACK_PTR,
                   trusted_stack_ptr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TLS_IDX, tls_idx);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_FPSCR, fpscr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_SYS_FPSCR, sys_fpscr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE1, tls_value1);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_SYSCALL_ROUTINE, syscall_routine);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_GUARD_TOKEN, guard_token);

#undef NACL_CHECK_FIELD
}

EXTERN_C_END

#endif /* !defined(__ASSEMBLER__) */

#endif /* __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_ARM_SEL_RT_H___ */

/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Secure Runtime
 */

#ifndef __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_MIPS_SEL_RT_H__
#define __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_MIPS_SEL_RT_H__ 1

#if !defined(__ASSEMBLER__)

#include <stddef.h>

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"

EXTERN_C_BEGIN

uint32_t NaClGetStackPtr(void);

typedef uint32_t nacl_reg_t;

#define NACL_PRIdNACL_REG NACL_PRId32
#define NACL_PRIiNACL_REG NACL_PRIi32
#define NACL_PRIoNACL_REG NACL_PRIo32
#define NACL_PRIuNACL_REG NACL_PRIu32
#define NACL_PRIxNACL_REG NACL_PRIx32
#define NACL_PRIXNACL_REG NACL_PRIX32

/*
 * NOTE: This struct needs to be synchronized with NACL_CALLEE_SAVE_LIST
 */

struct NaClThreadContext {
  nacl_reg_t  s0, s1, s2, s3, s4, s5, s6, s7, t8;
  /*           0   4   8   c  10  14  18  1c  20 */

  nacl_reg_t  stack_ptr, frame_ptr, prog_ctr;
  /*                 24         28        2c */

  /*
   * sys_ret and new_prog_ctr are not a part of the thread's register set,
   * but are needed by NaClSwitch. By including them here, the two
   * use the same interface.
   */
  uint32_t  sysret;
  /*            30 */
  uint32_t  new_prog_ctr;
  /*            34 */
  uint32_t  trusted_stack_ptr;
  /*            38 */
  uint32_t  tls_idx;
  /*            3c */
  uint32_t  tls_value1;
  /*            40 */
  uint32_t  tls_value2;
  /*            44 */
  uint32_t  guard_token;
  /*            48 */
};

static INLINE uintptr_t NaClGetThreadCtxSp(struct NaClThreadContext *th_ctx) {
  return th_ctx->stack_ptr;
}

NORETURN void NaClStartSwitch(struct NaClThreadContext *);

#endif /* !defined(__ASSEMBLER__) */

#define NACL_THREAD_CONTEXT_OFFSET_S0                  0x00
#define NACL_THREAD_CONTEXT_OFFSET_S1                  0x04
#define NACL_THREAD_CONTEXT_OFFSET_S2                  0x08
#define NACL_THREAD_CONTEXT_OFFSET_S3                  0x0c
#define NACL_THREAD_CONTEXT_OFFSET_S4                  0x10
#define NACL_THREAD_CONTEXT_OFFSET_S5                  0x14
#define NACL_THREAD_CONTEXT_OFFSET_S6                  0x18
#define NACL_THREAD_CONTEXT_OFFSET_S7                  0x1c
#define NACL_THREAD_CONTEXT_OFFSET_T8                  0x20
#define NACL_THREAD_CONTEXT_OFFSET_STACK_PTR           0x24
#define NACL_THREAD_CONTEXT_OFFSET_FRAME_PTR           0x28
#define NACL_THREAD_CONTEXT_OFFSET_PROG_CTR            0x2c
#define NACL_THREAD_CONTEXT_OFFSET_SYSRET              0x30
#define NACL_THREAD_CONTEXT_OFFSET_NEW_PROG_CTR        0x34
#define NACL_THREAD_CONTEXT_OFFSET_TRUSTED_STACK_PTR   0x38
#define NACL_THREAD_CONTEXT_OFFSET_TLS_IDX             0x3c
#define NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE1          0x40
#define NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE2          0x44
#define NACL_THREAD_CONTEXT_OFFSET_GUARD_TOKEN         0x48

#if !defined(__ASSEMBLER__)

/*
 * This function exists as a function only because compile-time
 * assertions need to be inside a function.  This function does not
 * need to be called for the assertions to be checked.
 */
static INLINE void NaClThreadContextOffsetCheck(void) {
  int offset = 0;

#define NACL_CHECK_FIELD(offset_name, field) \
    NACL_COMPILE_TIME_ASSERT(offset_name == \
                             offsetof(struct NaClThreadContext, field)); \
    CHECK(offset == offset_name); \
    offset += sizeof(((struct NaClThreadContext *) NULL)->field);

  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_S0, s0);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_S1, s1);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_S2, s2);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_S3, s3);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_S4, s4);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_S5, s5);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_S6, s6);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_S7, s7);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_T8, t8);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_STACK_PTR, stack_ptr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_FRAME_PTR, frame_ptr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_PROG_CTR, prog_ctr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_SYSRET, sysret);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_NEW_PROG_CTR, new_prog_ctr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TRUSTED_STACK_PTR,
                   trusted_stack_ptr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TLS_IDX, tls_idx);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE1, tls_value1);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TLS_VALUE2, tls_value2);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_GUARD_TOKEN, guard_token);
  CHECK(offset == sizeof(struct NaClThreadContext));

#undef NACL_CHECK_FIELD
}

EXTERN_C_END

#endif /* !defined(__ASSEMBLER__) */

#endif /* __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_MIPS_SEL_RT_H___ */

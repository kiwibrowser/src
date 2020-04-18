/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Secure Runtime
 */

#ifndef __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_32_SEL_RT_32_H__
#define __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_32_SEL_RT_32_H__ 1

/* This file can be #included from assembly to get the #defines. */
#if !defined(__ASSEMBLER__)

#include <stddef.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"

EXTERN_C_BEGIN

uint16_t NaClGetGlobalCs(void);

uint16_t NaClGetGlobalDs(void);

uint16_t NaClGetCs(void);
/* setting CS is done using an lcall */

uint16_t NaClGetDs(void);

void    NaClSetDs(uint16_t v);

uint16_t NaClGetEs(void);

void    NaClSetEs(uint16_t v);

uint16_t NaClGetFs(void);

void    NaClSetFs(uint16_t v);

uint16_t NaClGetGs(void);

void    NaClSetGs(uint16_t v);

uint16_t NaClGetSs(void);

void    NaClSyscallSegSSE(void);
void    NaClSyscallSegNoSSE(void);
void    NaClSyscallSegRegsSavedSSE(void);
void    NaClSyscallSegRegsSavedNoSSE(void);

/*
 * On a context switch through the syscall interface, not all
 * registers are saved.  We assume that C calling convention is used,
 * so %ecx and %edx are caller-saved and the NaCl service runtime does
 * not have to bother saving them; %eax (or %edx:%eax pair) should
 * have the return value, so its old value is also not saved.  (We
 * should, however, ensure that there is not an accidental covert
 * channel leaking information via these registers on syscall return.)
 * The eflags register is also caller saved.
 *
 * We assume that the following is packed.  This is true for gcc and
 * msvc for x86, but we will include a check that sizeof(struct
 * NaClThreadContext) == 64 bytes. (32-bit and 64-bit mode)
 */

typedef uint32_t  nacl_reg_t;  /* general purpose register type */

#define NACL_PRIdNACL_REG NACL_PRId32
#define NACL_PRIiNACL_REG NACL_PRIi32
#define NACL_PRIoNACL_REG NACL_PRIo32
#define NACL_PRIuNACL_REG NACL_PRIu32
#define NACL_PRIxNACL_REG NACL_PRIx32
#define NACL_PRIXNACL_REG NACL_PRIX32

/* The %gs segment points to this struct when running untrusted code. */
struct NaClGsSegment {
  uint32_t tls_value1;  /* Used by user code */
  uint32_t tls_value2;  /* Used by the integrated runtime (IRT) library */

  /*
   * These are register values to restore when returning to untrusted
   * code using NaClSwitchRemainingRegs().
   */
  uint32_t new_prog_ctr;
  uint32_t new_ecx;
};

/*
 * The layout of NaClThreadContext must be kept in sync with the
 * #defines below.
 */
#if NACL_WINDOWS
/* Align gs_segment for better performance on Intel Atom */
__declspec(align(64))
#endif
struct NaClThreadContext {
  /*
   * We align gs_segment to a multiple of 64 bytes because otherwise
   * memory accesses through the %gs segment are slow on Intel Atom
   * CPUs.
   *
   * While GCC allows __attribute__((aligned(X))) on a field in a
   * struct, MSVC does not, so we use a struct-level attribute (plus a
   * padding field) on all platforms for consistency.  Note that we do
   * not put __attribute__((aligned(X))) on struct NaClGsSegment
   * because that would increase sizeof(struct NaClGsSegment) to 64.
   *
   * By putting this field first in the struct, we avoid the need
   * for alignment padding before it.
   */
  struct NaClGsSegment gs_segment;

  /*
   * We need some padding somewhere to make the whole struct a multiple
   * of its 64-byte alignment size.  Putting it here rather than at the
   * end leaves space to extend struct NaClGsSegment without affecting
   * all the other offsets.
   *
   * As well as requiring the tedious updating of NACL_THREAD_CONTEXT_OFFSET_*
   * macros below, changing offsets when not also touching every .S file that
   * uses those macros runs afoul of a Gyp bug wherein incremental builds fail
   * to rebuild .S files whose included files have changed.
   * See http://code.google.com/p/nativeclient/issues/detail?id=2969
   */
  uint8_t     future_padding[0x24];

  /* ecx, edx, eax, eflags not saved */
  nacl_reg_t  ebx;
  nacl_reg_t  esi;
  nacl_reg_t  edi;
  nacl_reg_t  prog_ctr;  /* return addr */
  nacl_reg_t  frame_ptr;
  nacl_reg_t  stack_ptr;
  uint16_t    ss; /* stack_ptr and ss must be adjacent */
  uint16_t    fcw;
  uint16_t    sys_fcw;
  uint16_t    align_padding1;
  uint32_t    mxcsr;
  uint32_t    sys_mxcsr;
  /*
   * gs is our TLS base in the app; on the host side it's either fs or gs.
   */
  uint16_t    ds;
  uint16_t    es;
  uint16_t    fs;
  uint16_t    gs;
  /*
   * spring_addr, sys_ret and new_prog_ctr are not a part of the
   * thread's register set, but are needed by NaClSwitch.  By
   * including them here, the two use the same interface.
   */
  nacl_reg_t  new_prog_ctr;
  nacl_reg_t  sysret;
  nacl_reg_t  spring_addr;
  uint16_t    cs; /* spring_addr and cs must be adjacent */
  uint16_t    align_padding2;

  /* These two are adjacent because they are restored using 'lss'. */
  uint32_t    trusted_stack_ptr;
  uint16_t    trusted_ss;

  uint16_t    trusted_es;
  uint16_t    trusted_fs;
  uint16_t    trusted_gs;
}
#if defined(__GNUC__)
    /* Align gs_segment for better performance on Intel Atom */
    __attribute__((aligned(64)))
#endif
    ;

static INLINE uintptr_t NaClGetThreadCtxSp(struct NaClThreadContext *th_ctx) {
  return th_ctx->stack_ptr;
}

#endif /* !defined(__ASSEMBLER__) */

#define NACL_THREAD_CONTEXT_OFFSET_GS_SEGMENT         0x00
#define NACL_THREAD_CONTEXT_OFFSET_FUTURE_PADDING     0x10
#define NACL_THREAD_CONTEXT_OFFSET_EBX                0x34
#define NACL_THREAD_CONTEXT_OFFSET_ESI                0x38
#define NACL_THREAD_CONTEXT_OFFSET_EDI                0x3c
#define NACL_THREAD_CONTEXT_OFFSET_PROG_CTR           0x40
#define NACL_THREAD_CONTEXT_OFFSET_FRAME_PTR          0x44
#define NACL_THREAD_CONTEXT_OFFSET_STACK_PTR          0x48
#define NACL_THREAD_CONTEXT_OFFSET_SS                 0x4c
#define NACL_THREAD_CONTEXT_OFFSET_FCW                0x4e
#define NACL_THREAD_CONTEXT_OFFSET_SYS_FCW            0x50
#define NACL_THREAD_CONTEXT_OFFSET_ALIGN_PADDING1     0x52
#define NACL_THREAD_CONTEXT_OFFSET_MXCSR              0x54
#define NACL_THREAD_CONTEXT_OFFSET_SYS_MXCSR          0x58
#define NACL_THREAD_CONTEXT_OFFSET_DS                 0x5c
#define NACL_THREAD_CONTEXT_OFFSET_ES                 0x5e
#define NACL_THREAD_CONTEXT_OFFSET_FS                 0x60
#define NACL_THREAD_CONTEXT_OFFSET_GS                 0x62
#define NACL_THREAD_CONTEXT_OFFSET_NEW_PROG_CTR       0x64
#define NACL_THREAD_CONTEXT_OFFSET_SYSRET             0x68
#define NACL_THREAD_CONTEXT_OFFSET_SPRING_ADDR        0x6c
#define NACL_THREAD_CONTEXT_OFFSET_CS                 0x70
#define NACL_THREAD_CONTEXT_OFFSET_ALIGN_PADDING2     0x72
#define NACL_THREAD_CONTEXT_OFFSET_TRUSTED_STACK_PTR  0x74
#define NACL_THREAD_CONTEXT_OFFSET_TRUSTED_SS         0x78
#define NACL_THREAD_CONTEXT_OFFSET_TRUSTED_ES         0x7a
#define NACL_THREAD_CONTEXT_OFFSET_TRUSTED_FS         0x7c
#define NACL_THREAD_CONTEXT_OFFSET_TRUSTED_GS         0x7e

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

  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_GS_SEGMENT, gs_segment);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_FUTURE_PADDING, future_padding);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_EBX, ebx);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_ESI, esi);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_EDI, edi);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_PROG_CTR, prog_ctr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_FRAME_PTR, frame_ptr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_STACK_PTR, stack_ptr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_SS, ss);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_FCW, fcw);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_SYS_FCW, sys_fcw);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_ALIGN_PADDING1, align_padding1);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_MXCSR, mxcsr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_SYS_MXCSR, sys_mxcsr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_DS, ds);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_ES, es);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_FS, fs);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_GS, gs);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_NEW_PROG_CTR, new_prog_ctr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_SYSRET, sysret);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_SPRING_ADDR, spring_addr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_CS, cs);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_ALIGN_PADDING2, align_padding2);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TRUSTED_STACK_PTR,
                   trusted_stack_ptr);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TRUSTED_SS, trusted_ss);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TRUSTED_ES, trusted_es);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TRUSTED_FS, trusted_fs);
  NACL_CHECK_FIELD(NACL_THREAD_CONTEXT_OFFSET_TRUSTED_GS, trusted_gs);
  CHECK(offset == sizeof(struct NaClThreadContext));

#undef NACL_CHECK_FIELD
}

EXTERN_C_END

#endif

#endif /* __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_32_SEL_RT_32_H__ */

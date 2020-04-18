/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Store instruction written in raw assembly so we know they get implemented
 * with a single instruction
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_asm.h"


#if NACL_BUILD_SUBARCH == 32

/*
 * use cdecl calling convention
 * dst = 0x4(%esp)
 * src = 0x8(%esp)
 */

DEFINE_GLOBAL_HIDDEN_FUNCTION(onestore_memmove4):
  /*
   * gcc generates a single store for this, but it would be scary
   * to rely on that
   */
  movl 0x8(%esp),%eax
  movl 0x4(%esp),%edx
  movl (%eax),%eax
  movl %eax,(%edx)
  ret
  .align 8

DEFINE_GLOBAL_HIDDEN_FUNCTION(onestore_memmove8):
  /* gcc generates two stores for this, use SSE registers instead */
  movl   0x8(%esp),%eax
  movl   0x4(%esp),%edx
  movq   (%eax),%xmm0
  movq   %xmm0,(%edx)
  ret
  .align 8

#elif NACL_BUILD_SUBARCH == 64

/*
 * TODO(jansel): Note that the 64 bit versions here are unreachable
 * because NaClCodeCopy LOG_FATALs on 64 bit.  We include 64 bit
 * implementations for completeness, but they need to be tested more once
 * 64 bit is enabled.
 */

#if NACL_WINDOWS
/* WIN64 uses its own calling convention */
# define DST %rcx
# define SRC %rdx
#else
# define DST %rdi
# define SRC %rsi
#endif

DEFINE_GLOBAL_HIDDEN_FUNCTION(onestore_memmove4):
  /*
   * gcc generates a single store for this, but it would be terrifying
   * to rely on that
   */
  movl   (SRC), %eax
  movl   %eax, (DST)
  retq
  .align 8

DEFINE_GLOBAL_HIDDEN_FUNCTION(onestore_memmove8):
  /*
   * gcc generates a single store for this, but it would be frightful
   * to rely on that
   */
  movq   (SRC), %rax
  movq   %rax, (DST)
  retq
  .align 8

#else
# error "Unsupported architecture"
#endif


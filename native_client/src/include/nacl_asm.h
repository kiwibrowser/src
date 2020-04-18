/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_NACL_ASM_H_
#define NATIVE_CLIENT_SRC_INCLUDE_NACL_ASM_H_

#include "native_client/src/include/build_config.h"

/*
 * macros to provide uniform access to identifiers from assembly due
 * to different C -> asm name mangling conventions and other platform-specific
 * requirements
 */
#if NACL_OSX
# define IDENTIFIER(n)  _##n
#elif NACL_LINUX
# define IDENTIFIER(n)  n
#elif NACL_WINDOWS
# if defined(_WIN64)
#   define IDENTIFIER(n)  n
# else
#   define IDENTIFIER(n)  _##n
# endif
#elif defined(__native_client__)
# define IDENTIFIER(n)  n
#else
# error "Unrecognized OS"
#endif

#if NACL_OSX
# define HIDDEN(n)  .private_extern IDENTIFIER(n)
#elif NACL_LINUX
# define HIDDEN(n)  .hidden IDENTIFIER(n)
#elif NACL_WINDOWS
/* On Windows, symbols are hidden by default. */
# define HIDDEN(n)
#elif defined(__native_client__)
# define HIDDEN(n)  .hidden IDENTIFIER(n)
#else
# error "Unrecognized OS"
#endif

/*
 * ARM requires .type XXX, %function to ensure proper switching between
 * Thumb and ARM instruction sets on ELF-based platforms.  We do not use
 * '.type' globally because OSX and Windows are not ELF and do not support it.
 */
#if defined(__ELF__)

#define DEFINE_GLOBAL_HIDDEN_DATA(n) \
  .globl IDENTIFIER(n); HIDDEN(n); .type IDENTIFIER(n), %object; IDENTIFIER(n)

#define DEFINE_GLOBAL_HIDDEN_FUNCTION(n) \
  .globl IDENTIFIER(n); HIDDEN(n); .type IDENTIFIER(n), %function; IDENTIFIER(n)

#else

#define DEFINE_GLOBAL_HIDDEN_DATA(n) \
  .globl IDENTIFIER(n); HIDDEN(n); IDENTIFIER(n)

#define DEFINE_GLOBAL_HIDDEN_FUNCTION(n) \
  .globl IDENTIFIER(n); HIDDEN(n); IDENTIFIER(n)

#endif

#define DEFINE_GLOBAL_HIDDEN_LOCATION(n) \
  .globl IDENTIFIER(n); HIDDEN(n); IDENTIFIER(n)

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_NACL_ASM_H_ */

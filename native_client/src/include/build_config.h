/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * All macros defined in this file should be checked by the presubmit check in
 * tools/code_hygiene.py to ensure that this header is included in source files
 * when needed.
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_BUILD_CONFIG_H_
#define NATIVE_CLIENT_SRC_INCLUDE_BUILD_CONFIG_H_ 1

#include "native_client/src/include/nacl_base.h"

#if !defined(NACL_WINDOWS) && !defined(NACL_LINUX) && !defined(NACL_OSX) && \
    !defined(NACL_ANDROID)

#if defined(_WIN32)
# define NACL_WINDOWS 1
#else
# define NACL_WINDOWS 0
#endif

#if defined(__linux__)
# define NACL_LINUX 1
#else
# define NACL_LINUX 0
#endif

#if defined(__APPLE__)
# define NACL_OSX 1
#else
# define NACL_OSX 0
#endif

#if defined(ANDROID)
# define NACL_ANDROID 1
#else
# define NACL_ANDROID 0
#endif

#endif

/* TODO(teravest): Remove this guard when builds stop defining these. */
#if defined(NACL_BUILD_ARCH) && NACL_ARCH(NACL_BUILD_ARCH) != NACL_pnacl && \
    !defined(NACL_BUILD_SUBARCH)
# error Please define both NACL_BUILD_ARCH and NACL_BUILD_SUBARCH.
#elif !defined(NACL_BUILD_ARCH) && defined(NACL_BUILD_SUBARCH)
# error Please define both NACL_BUILD_ARCH and NACL_BUILD_SUBARCH.
#endif


#if !defined(NACL_BUILD_ARCH)

#if defined(_M_X64) || defined(__x86_64__)
# define NACL_BUILD_ARCH x86
# define NACL_BUILD_SUBARCH 64
#endif

#if defined(_M_IX86) || defined(__i386__)
# define NACL_BUILD_ARCH x86
# define NACL_BUILD_SUBARCH 32
#endif

#if defined(__ARMEL__)
# define NACL_BUILD_ARCH arm
# define NACL_BUILD_SUBARCH 32
#endif

#if defined(__MIPSEL__)
# define NACL_BUILD_ARCH mips
# define NACL_BUILD_SUBARCH 32
#endif

#endif  /* !defined(NACL_BUILD_ARCH) */

/*
 * TODO(teravest): Require NACL_BUILD_ARCH and NACL_BUILD_SUBARCH to be defined
 * once they're defined for the pnacl translator build.
 */

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_BUILD_CONFIG_H_ */

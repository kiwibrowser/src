/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  IMC API.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_MMAN_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_MMAN_H_

#include <sys/types.h>

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"

#if defined(NACL_IN_TOOLCHAIN_HEADERS)

#ifdef __cplusplus
extern "C" {
#endif

/** Description of mmap. More details... */
extern void *mmap(void *start, size_t length, int prot, int flags,
                  int desc, off_t offset);

/** Description of munmap. More details... */
extern int munmap(void *start, size_t length);

/** Description of mprotect. More details... */
extern int mprotect(void *start, size_t length, int prot);

#ifdef __cplusplus
}
#endif

#endif  // defined(NACL_IN_TOOLCHAIN_HEADERS)

#endif

/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl kernel / service run-time system call ABI.  stat/fstat.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_STAT_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_STAT_H_

#include "native_client/src/trusted/service_runtime/include/bits/stat.h"

#if defined(NACL_IN_TOOLCHAIN_HEADERS)

#ifdef __cplusplus
extern "C" {
#endif

extern int stat(char const *path, struct nacl_abi_stat *stbuf);
extern int fstat(int d, struct nacl_abi_stat *stbuf);
extern int lstat(const char *path, struct nacl_abi_stat *stbuf);
extern int fstatat(int dirfd, const char *pathname,
                   struct nacl_abi_stat *stbuf, int flags);
extern int mkdir(const char *path, nacl_abi_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif

#endif

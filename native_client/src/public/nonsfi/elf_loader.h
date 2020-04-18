/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_PUBLIC_NONSFI_ELF_LOADER_H_
#define NATIVE_CLIENT_SRC_PUBLIC_NONSFI_ELF_LOADER_H_ 1

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Loads the ELF binary from the given file descriptor.
 * This takes the ownership of the given fd, so a caller does not need to
 * close it.
 * On error, this function crashes with NaClLog(LOG_FATAL).
 */
uintptr_t NaClLoadElfFile(int fd);

EXTERN_C_END

#endif

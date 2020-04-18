/*
 * Copyright (c) 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_NONSFI_IRT_IRT_ICACHE_H_
#define NATIVE_CLIENT_SRC_NONSFI_IRT_IRT_ICACHE_H_ 1

#include <stddef.h>
#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

int irt_clear_cache(void *addr, size_t size);

EXTERN_C_END

#endif

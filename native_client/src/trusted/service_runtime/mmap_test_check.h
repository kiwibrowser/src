/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_MMAP_TEST_CHECK_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_MMAP_TEST_CHECK_H_

#include <stdlib.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

/*
 * Checks that the memory mapping at given address has the properties
 * described by the remaining arguments.
 */
#if NACL_WINDOWS
void CheckMapping(uintptr_t addr, size_t size, int state, int protect,
                  int map_type);
void CheckGuardMapping(uintptr_t addr, size_t size, int state, int protect,
                       int map_type);
#else
void CheckMapping(uintptr_t addr, size_t size, int protect, int map_type);
#endif

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_MMAP_TEST_CHECK_H_ */

/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_VALGRIND_HOOKS_H__
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_VALGRIND_HOOKS_H__ 1

#include <stdlib.h>
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

/*
 * When running under Valgrind, calls to these functions are intercepted and
 * used to understand memory layout of the NaCl process and locate debug info
 * for the untrusted binary (or binaries).
 *
 * It is important that calls to these functions are not inlined, hence a
 * separate file.
 */

/* Tells Valgrind about a new untrusted memory mapping. */
void NaClFileMappingForValgrind(size_t vma, size_t size, size_t file_offset);

/* Tells Valgrind the path to the nexe, if it is known. */
void NaClFileNameForValgrind(const char* name);

/* Tells Valgrind the base address of the sandbox. */
void NaClSandboxMemoryStartForValgrind(uintptr_t mem_start);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_VALGRIND_HOOKS_H__ */

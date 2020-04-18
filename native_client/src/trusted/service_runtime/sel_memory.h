/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL) memory protection abstractions.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_MEMORY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_MEMORY_H_ 1

#include "native_client/src/include/nacl_compiler_annotations.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int NaClPageAlloc(void **p, size_t num_bytes) NACL_WUR;

int NaClPageAllocRandomized(void **p, size_t num_bytes) NACL_WUR;

int NaClPageAllocAtAddr(void **p, size_t num_bytes) NACL_WUR;

void NaClPageFree(void *p, size_t num_bytes);

int NaClMprotect(void *addr, size_t len, int prot) NACL_WUR;

int NaClMadvise(void *start, size_t length, int advice) NACL_WUR;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*  NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_MEMORY_H_ */

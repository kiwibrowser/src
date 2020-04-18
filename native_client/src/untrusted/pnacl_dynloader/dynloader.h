/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_UNTRUSTED_PNACL_DYNLOADER_DYNLOADER_H_
#define NATIVE_CLIENT_SRC_UNTRUSTED_PNACL_DYNLOADER_DYNLOADER_H_ 1

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Dynamically loads an ELF DSO that was produced by the PNaCl translator
 * from a PSO.
 *
 * Returns an errno value, or 0 on success.
 *
 * On success, this sets |*pso_root| to the address of the PSO's
 * __pnacl_pso_root variable in memory.
 */
int pnacl_load_elf_file(const char *filename, void **pso_root);

EXTERN_C_END

#endif

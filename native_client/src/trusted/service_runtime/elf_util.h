/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ELF_UTIL_H__
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ELF_UTIL_H__ 1

#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

struct NaClElfImage;
struct NaClValidationMetadata;

uintptr_t NaClElfImageGetEntryPoint(struct NaClElfImage *image);

struct NaClElfImage *NaClElfImageNew(struct NaClDesc *gp,
                                     NaClErrorCode *err_code);

NaClErrorCode NaClElfImageValidateElfHeader(struct NaClElfImage *image);

struct NaClElfImageInfo {
  uintptr_t static_text_end;
  uintptr_t rodata_start;
  uintptr_t rodata_end;
  uintptr_t data_start;
  uintptr_t data_end;
  uintptr_t max_vaddr;
};

NaClErrorCode NaClElfImageValidateProgramHeaders(
  struct NaClElfImage *image,
  uint8_t             addr_bits,
  struct NaClElfImageInfo *info);

/*
 * Loads an ELF executable before the address space's memory
 * protections have been set up by NaClMemoryProtection().
 */
NaClErrorCode NaClElfImageLoad(struct NaClElfImage *image,
                               struct NaClDesc *ndp,
                               struct NaClApp *nap);

/*
 * Loads an ELF object after NaClMemoryProtection() has been called.
 */
NaClErrorCode NaClElfImageLoadDynamically(
    struct NaClElfImage *image,
    struct NaClApp *nap,
    struct NaClDesc *gfile,
    struct NaClValidationMetadata *metadata);

void NaClElfImageDelete(struct NaClElfImage *image);


#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ELF_UTIL_H__ */

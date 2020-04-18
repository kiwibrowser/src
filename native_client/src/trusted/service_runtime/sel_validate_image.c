/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/concurrency_ops.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/validator/ncvalidate.h"

const size_t kMinimumCachedCodeSize = 40000;

/* Translate validation status to values wanted by sel_ldr. */
static int NaClValidateStatus(NaClValidationStatus status) {
  switch (status) {
    case NaClValidationSucceeded:
      return LOAD_OK;
    case NaClValidationFailedOutOfMemory:
      /* Note: this is confusing, but is what sel_ldr is expecting. */
      return LOAD_BAD_FILE;
    case NaClValidationFailed:
    case NaClValidationFailedNotImplemented:
    case NaClValidationFailedCpuNotSupported:
    case NaClValidationFailedSegmentationIssue:
    default:
      return LOAD_VALIDATION_FAILED;
  }
}

int NaClValidateCode(struct NaClApp *nap, uintptr_t guest_addr,
                     uint8_t *data, size_t size,
                     const struct NaClValidationMetadata *metadata) {
  NaClValidationStatus status = NaClValidationSucceeded;
  struct NaClValidationCache *cache = nap->validation_cache;
  const struct NaClValidatorInterface *validator = nap->validator;
  uint32_t flags = nap->pnacl_mode ? NACL_DISABLE_NONTEMPORALS_X86 : 0;

  if (size < kMinimumCachedCodeSize) {
    /*
     * Don't cache the validation of small code chunks for three reasons:
     * 1) The size of the validation cache will be bounded.  Cache entries are
     *    better used for bigger code.
     * 2) The per-transaction overhead of validation caching is more noticeable
     *    for small code.
     * 3) JITs tend to generate a lot of small code chunks, and JITed code may
     *    never be seen again.  Currently code size is the best mechanism we
     *    have for heuristically distinguishing between JIT and static code.
     *    (In practice most Mono JIT blocks are less than 1k, and a quick look
     *    didn't show any above 35k.)
     * The choice of what constitutes "small" is arbitrary, and should be
     * empirically tuned.
     * TODO(ncbray) let the syscall specify if the code is cached or not.
     */
    metadata = NULL;
    cache = NULL;
  }

  if (nap->validator_stub_out_mode) {
    /* Validation caching is currently incompatible with stubout. */
    metadata = NULL;
    cache = NULL;
    /* In stub out mode, we do two passes.  The second pass acts as a
       sanity check that bad instructions were indeed overwritten with
       allowable HLTs. */
    status = validator->Validate(guest_addr, data, size,
                                 TRUE, /* stub out */
                                 flags,
                                 FALSE, /* text is not read-only */
                                 nap->cpu_features,
                                 metadata,
                                 cache);
  }
  if (status == NaClValidationSucceeded) {
    status = validator->Validate(guest_addr, data, size,
                                 FALSE, /* do not stub out */
                                 flags,
                                 FALSE /* readonly_text */,
                                 nap->cpu_features,
                                 metadata,
                                 cache);
  }
  return NaClValidateStatus(status);
}

int NaClValidateCodeReplacement(struct NaClApp *nap, uintptr_t guest_addr,
                                uint8_t *data_old, uint8_t *data_new,
                                size_t size) {
  if (nap->validator_stub_out_mode) return LOAD_BAD_FILE;

  if ((guest_addr % nap->bundle_size) != 0 ||
      (size % nap->bundle_size) != 0) {
    return LOAD_BAD_FILE;
  }

  return NaClValidateStatus(nap->validator->ValidateCodeReplacement(
      guest_addr, data_old, data_new, size, nap->cpu_features));
}

int NaClCopyCode(struct NaClApp *nap, uintptr_t guest_addr,
                 uint8_t *data_old, uint8_t *data_new,
                 size_t size) {
  int status;
  status = NaClValidateStatus(nap->validator->CopyCode(
                              guest_addr, data_old, data_new, size,
                              nap->cpu_features,
                              NaClCopyInstruction));
  /*
   * Flush the processor's instruction cache.  This is not necessary
   * for security, because any old cached instructions will just be
   * safe halt instructions.  It is only necessary to ensure that
   * untrusted code runs correctly when it tries to execute the
   * dynamically-loaded code.
   */
  NaClFlushCacheForDoublyMappedCode(data_old,
                                    (uint8_t *) guest_addr,
                                    size);
  return status;
}

NaClErrorCode NaClValidateImage(struct NaClApp  *nap) {
  uintptr_t               memp;
  uintptr_t               endp;
  size_t                  regionsize;
  NaClErrorCode           rcode;

  memp = nap->mem_start + NACL_TRAMPOLINE_END;
  endp = nap->mem_start + nap->static_text_end;
  regionsize = endp - memp;
  if (endp < memp) {
    return LOAD_NO_MEMORY;
  }

  if (nap->skip_validator) {
    NaClLog(LOG_ERROR, "VALIDATION SKIPPED.\n");
    return LOAD_OK;
  } else {
    /* TODO(ncbray) metadata for the main image. */
    rcode = NaClValidateCode(nap, NACL_TRAMPOLINE_END,
                             (uint8_t *) memp, regionsize, NULL);
    if (LOAD_OK != rcode) {
      if (nap->ignore_validator_result) {
        NaClLog(LOG_ERROR, "VALIDATION FAILED: continuing anyway...\n");
        rcode = LOAD_OK;
      } else {
        NaClLog(LOG_ERROR, "VALIDATION FAILED.\n");
        NaClLog(LOG_ERROR,
                "Run sel_ldr in debug mode to ignore validation failure.\n");
        NaClLog(LOG_ERROR,
                "Run ncval <module-name> for validation error details.\n");
        rcode = LOAD_VALIDATION_FAILED;
      }
    }
  }
  return rcode;
}

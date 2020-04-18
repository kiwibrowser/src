/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Implement the Validator API for the x86-32 architecture. */
#include <errno.h>
#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/validator/validation_cache.h"
#include "native_client/src/trusted/validator_ragel/bitmap.h"
#include "native_client/src/trusted/validator_ragel/dfa_validate_common.h"
#include "native_client/src/trusted/validator_ragel/validator.h"

/*
 * Be sure the correct compile flags are defined for this.
 * TODO(khim): Try to figure out if these checks actually make any sense.
 *             I don't foresee any use in cross-environment, but it should work
 *             and may be useful in some case so why not?
 */
#if NACL_ARCH(NACL_BUILD_ARCH) != NACL_x86 || NACL_BUILD_SUBARCH != 32
# error "Can't compile, target is for x86-32"
#endif

NaClValidationStatus ApplyDfaValidator_x86_32(
    uintptr_t guest_addr,
    uint8_t *data,
    size_t size,
    int stubout_mode,
    uint32_t flags,
    int readonly_text,
    const NaClCPUFeatures *f,
    const struct NaClValidationMetadata *metadata,
    struct NaClValidationCache *cache) {
  /* TODO(jfb) Use a safe cast here. */
  NaClCPUFeaturesX86 *cpu_features = (NaClCPUFeaturesX86 *) f;
  enum NaClValidationStatus status = NaClValidationFailed;
  void *query = NULL;
  struct StubOutCallbackData callback_data;
  callback_data.flags = flags;
  callback_data.chunk_begin = data;
  callback_data.chunk_end = data + size;
  callback_data.cpu_features = cpu_features;
  callback_data.validate_chunk_func = ValidateChunkIA32;
  callback_data.did_rewrite = 0;

  UNREFERENCED_PARAMETER(guest_addr);

  if (stubout_mode)
    return NaClValidationFailedNotImplemented;
  if (!NaClArchSupportedX86(cpu_features))
    return NaClValidationFailedCpuNotSupported;
  if (size & kBundleMask)
    return NaClValidationFailed;

  /*
   * If the validation caching interface is available and it would be
   * inexpensive to do so, perform a query.
   */
  if (cache != NULL && NaClCachingIsInexpensive(cache, metadata))
    query = cache->CreateQuery(cache->handle);
  if (query != NULL) {
    const char validator_id[] = "x86-32 dfa";
    cache->AddData(query, (uint8_t *) validator_id, sizeof(validator_id));
    cache->AddData(query, (uint8_t *) cpu_features, sizeof(*cpu_features));
    NaClAddCodeIdentity(data, size, metadata, cache, query);
    if (cache->QueryKnownToValidate(query)) {
      cache->DestroyQuery(query);
      return NaClValidationSucceeded;
    }
  }

  if (readonly_text) {
    if (ValidateChunkIA32(data, size, 0 /*options*/, cpu_features,
                          NaClDfaProcessValidationError,
                          NULL))
      status = NaClValidationSucceeded;
  } else {
    if (ValidateChunkIA32(data, size, 0 /*options*/, cpu_features,
                          NaClDfaStubOutUnsupportedInstruction,
                          &callback_data))
      status = NaClValidationSucceeded;
  }
  if (status != NaClValidationSucceeded && errno == ENOMEM)
    status = NaClValidationFailedOutOfMemory;

  /* Cache the result if validation succeeded and the code was not modified. */
  if (query != NULL) {
    if (status == NaClValidationSucceeded && callback_data.did_rewrite == 0)
      cache->SetKnownToValidate(query);
    cache->DestroyQuery(query);
  }

  return status;
}


static NaClValidationStatus ValidatorCopy_x86_32(
    uintptr_t guest_addr,
    uint8_t *data_existing,
    uint8_t *data_new,
    size_t size,
    const NaClCPUFeatures *f,
    NaClCopyInstructionFunc copy_func) {
  /* TODO(jfb) Use a safe cast here. */
  NaClCPUFeaturesX86 *cpu_features = (NaClCPUFeaturesX86 *) f;
  struct CodeCopyCallbackData callback_data;
  UNREFERENCED_PARAMETER(guest_addr);

  if (size & kBundleMask)
    return NaClValidationFailed;
  callback_data.copy_func = copy_func;
  callback_data.existing_minus_new = data_existing - data_new;
  if (ValidateChunkIA32(data_new, size, CALL_USER_CALLBACK_ON_EACH_INSTRUCTION,
                        cpu_features, NaClDfaProcessCodeCopyInstruction,
                        &callback_data))
    return NaClValidationSucceeded;
  if (errno == ENOMEM)
    return NaClValidationFailedOutOfMemory;
  return NaClValidationFailed;
}

/* This structure is used by callbacks ProcessCodeReplacementInstruction
   and ProcessOriginalCodeInstruction during code replacement validation
   in ValidatorCodeReplacement_x86_32.  */
struct CodeReplacementCallbackData {
  /* Bitmap with boundaries of the instructions in the old code bundle.  */
  bitmap_word instruction_boundaries_existing;
  /* Bitmap with boundaries of the instructions in the new code bundle.  */
  bitmap_word instruction_boundaries_new;
  /* cpu_features - needed for the call to ValidateChunkIA32.  */
  const NaClCPUFeaturesX86 *cpu_features;
  /* Pointer to the start of the current bundle in the old code.  */
  const uint8_t *bundle_existing;
  /* Pointer to the start of the new code.  */
  const uint8_t *data_new;
  /* Difference between addresses: data_existing - data_new. This is needed for
     fast comparison between existing and new code.  */
  ptrdiff_t existing_minus_new;
};

static Bool ProcessOriginalCodeInstruction(const uint8_t *begin_existing,
                                           const uint8_t *end_existing,
                                           uint32_t info_existing,
                                           void *callback_data) {
  struct CodeReplacementCallbackData *data = callback_data;
  size_t instruction_length = end_existing - begin_existing;
  const uint8_t *begin_new = begin_existing - data->existing_minus_new;

  /* Sanity check: instruction must be no longer than 17 bytes.  */
  CHECK(instruction_length <= MAX_INSTRUCTION_LENGTH);

  /* Sanity check: old code must be valid... except for jumps.  */
  CHECK(!(info_existing &
          (VALIDATION_ERRORS_MASK & ~DIRECT_JUMP_OUT_OF_RANGE)));

  /*
   * Return failure if code replacement attempts to delete or modify a special
   * instruction such as naclcall or nacljmp.
   */
  if ((info_existing & SPECIAL_INSTRUCTION) &&
      memcmp(begin_new, begin_existing, instruction_length) != 0)
    return FALSE;

  BitmapSetBit(&data->instruction_boundaries_existing,
               begin_existing - data->bundle_existing);

  return TRUE;
}

static INLINE Bool ProcessCodeReplacementInstructionInfoFlags(
    const uint8_t *begin_existing,
    const uint8_t *begin_new,
    size_t instruction_length,
    uint32_t info_new,
    struct CodeReplacementCallbackData *data) {

  /* Unsupported instruction must have been replaced with HLTs.  */
  if ((info_new & VALIDATION_ERRORS_MASK) == CPUID_UNSUPPORTED_INSTRUCTION) {
    /*
     * The new instruction will be replaced with a bunch of HLTs by
     * ValidatorCopy_x86_32 and they are single-byte instructions.
     * Mark all the bytes as instruction boundaries.
     */
    if (NaClDfaCodeReplacementIsStubouted(begin_existing, instruction_length)) {
      BitmapSetBits(&data->instruction_boundaries_new,
                    (begin_new - data->data_new) & kBundleMask,
                    instruction_length);
      return TRUE;
    }
    return FALSE;
  }

  /* If we have jump which jumps out of its range...  */
  if (info_new & DIRECT_JUMP_OUT_OF_RANGE) {
    /* then everything is fine if it's the only error and jump is unchanged!  */
    if ((info_new & VALIDATION_ERRORS_MASK) == DIRECT_JUMP_OUT_OF_RANGE &&
        memcmp(begin_new, begin_existing, instruction_length) == 0)
      return TRUE;
    return FALSE;
  }

  /* If instruction is not accepted then we have nothing to do here.  */
  if (info_new & (VALIDATION_ERRORS_MASK | BAD_JUMP_TARGET))
    return FALSE;

  /* Instruction is untouched: we are done.  */
  if (memcmp(begin_new, begin_existing, instruction_length) == 0)
    return TRUE;

  /*
   * Return failure if code replacement attempts to create or modify a special
   * instruction such as naclcall or nacljmp.
   */
  if (info_new & SPECIAL_INSTRUCTION)
    return FALSE;

  /* No problems found.  */
  return TRUE;
}


static Bool ProcessCodeReplacementInstruction(const uint8_t *begin_new,
                                              const uint8_t *end_new,
                                              uint32_t info_new,
                                              void *callback_data) {
  struct CodeReplacementCallbackData *data = callback_data;
  size_t instruction_length = end_new - begin_new;
  const uint8_t *begin_existing = begin_new + data->existing_minus_new;

  /* Sanity check: instruction must be no longer than 17 bytes.  */
  CHECK(instruction_length <= MAX_INSTRUCTION_LENGTH);

  if (!ProcessCodeReplacementInstructionInfoFlags(begin_existing, begin_new,
                                                 instruction_length,
                                                 info_new, data))
    return FALSE;

  /* We mark the end of the instruction as valid jump target here.  */
  BitmapSetBit(&data->instruction_boundaries_new,
               (begin_new - data->data_new) & kBundleMask);

  /* If we at the end of a bundle then check boundaries.  */
  if (((end_new - data->data_new) & kBundleMask) == 0) {
    Bool rc;
    data->bundle_existing = end_new - kBundleSize + data->existing_minus_new;

    /* Check existing code and mark its instruction boundaries.  */
    rc = ValidateChunkIA32(data->bundle_existing, kBundleSize,
                           CALL_USER_CALLBACK_ON_EACH_INSTRUCTION,
                           data->cpu_features, ProcessOriginalCodeInstruction,
                           callback_data);
    /* Attempt to delete superinstruction is detected.  */
    if (!rc)
      return FALSE;

    if (data->instruction_boundaries_existing !=
        data->instruction_boundaries_new)
      return FALSE;
    /* Mark all boundaries in the next bundle invalid.  */
    data->instruction_boundaries_existing = 0;
    data->instruction_boundaries_new = 0;
  }

  return TRUE;
}

static NaClValidationStatus ValidatorCodeReplacement_x86_32(
    uintptr_t guest_addr,
    uint8_t *data_existing,
    uint8_t *data_new,
    size_t size,
    const NaClCPUFeatures *f) {
  /* TODO(jfb) Use a safe cast here. */
  NaClCPUFeaturesX86 *cpu_features = (NaClCPUFeaturesX86 *) f;
  struct CodeReplacementCallbackData callback_data;
  UNREFERENCED_PARAMETER(guest_addr);

  if (size & kBundleMask)
    return NaClValidationFailed;
  /* Mark all boundaries in the bundle invalid.  */
  callback_data.instruction_boundaries_existing = 0;
  callback_data.instruction_boundaries_new = 0;
  callback_data.cpu_features = cpu_features;
  /* Note: bundle_existing is used when we call second validator.  */
  callback_data.data_new = data_new;
  callback_data.existing_minus_new = data_existing - data_new;
  if (ValidateChunkIA32(data_new, size, CALL_USER_CALLBACK_ON_EACH_INSTRUCTION,
                        cpu_features, ProcessCodeReplacementInstruction,
                        &callback_data))
    return NaClValidationSucceeded;
  if (errno == ENOMEM)
    return NaClValidationFailedOutOfMemory;
  return NaClValidationFailed;
}


struct IsOnInstBoundaryCallbackData {
  int found_addr;
  const uint8_t *addr;
};

static Bool IsOnInstBoundaryCallback(const uint8_t *begin,
                                     const uint8_t *end,
                                     uint32_t info,
                                     void *callback_data) {
  struct IsOnInstBoundaryCallbackData *data = callback_data;
  UNREFERENCED_PARAMETER(info);
  UNREFERENCED_PARAMETER(end);

  /*
   * For x86-32 superinstructions are at most 2 normal instructions,
   * therefore every instruction reported will be a valid jump target.
   * This is not true for x86-64 and chained instructions.
   */
  if (begin == data->addr)
    data->found_addr = TRUE;

  /* We should never invalidate the DFA as a whole. */
  return TRUE;
}

static NaClValidationStatus IsOnInstBoundary_x86_32(
    uintptr_t guest_addr,
    uintptr_t addr,
    const uint8_t *data,
    size_t size,
    const NaClCPUFeatures *f) {
  NaClCPUFeaturesX86 *cpu_features = (NaClCPUFeaturesX86 *) f;
  struct IsOnInstBoundaryCallbackData callback_data;

  uint32_t guest_bundle_addr = (addr & ~kBundleMask);
  const uint8_t *local_bundle_addr = data + (guest_bundle_addr - guest_addr);
  int rc;

  /* Check code range doesn't overflow. */
  CHECK(guest_addr + size > guest_addr);
  CHECK(size % kBundleSize == 0 && size != 0);
  CHECK((uint32_t) (guest_addr & ~kBundleMask) == guest_addr);

  /* Check addr is within code boundary. */
  if (addr < guest_addr || addr >= guest_addr + size)
    return NaClValidationFailed;

  callback_data.found_addr = FALSE;
  callback_data.addr = data + (addr - guest_addr);

  rc = ValidateChunkIA32(local_bundle_addr, kBundleSize,
                         CALL_USER_CALLBACK_ON_EACH_INSTRUCTION,
                         cpu_features, IsOnInstBoundaryCallback,
                         &callback_data);

  CHECK(rc);

  return callback_data.found_addr ? NaClValidationSucceeded
                                  : NaClValidationFailed;
}

static const struct NaClValidatorInterface validator = {
  FALSE, /* Optional stubout_mode is not implemented.            */
  TRUE,  /* Optional readonly_text mode is implemented.          */
  TRUE,  /* Optional code replacement functions are implemented. */
  ApplyDfaValidator_x86_32,
  ValidatorCopy_x86_32,
  ValidatorCodeReplacement_x86_32,
  sizeof(NaClCPUFeaturesX86),
  NaClSetAllCPUFeaturesX86,
  NaClGetCurrentCPUFeaturesX86,
  IsOnInstBoundary_x86_32,
};

const struct NaClValidatorInterface *NaClDfaValidatorCreate_x86_32(void) {
  return &validator;
}

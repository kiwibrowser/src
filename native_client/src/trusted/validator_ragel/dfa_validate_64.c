/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Implement the Validator API for the x86-64 architecture. */
#include <errno.h>
#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/validator/validation_cache.h"
#include "native_client/src/trusted/validator_ragel/dfa_validate_common.h"
#include "native_client/src/trusted/validator_ragel/validator.h"

/*
 * Be sure the correct compile flags are defined for this.
 * TODO(khim): Try to figure out if these checks actually make any sense.
 *             I don't foresee any use in cross-environment, but it should work
 *             and may be useful in some case so why not?
 */
#if NACL_ARCH(NACL_BUILD_ARCH) != NACL_x86 || NACL_BUILD_SUBARCH != 64
# error "Can't compile, target is for x86-64"
#endif


static NaClValidationStatus ApplyDfaValidator_x86_64(
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
  callback_data.validate_chunk_func = ValidateChunkAMD64;
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
    const char validator_id[] = "x86-64 dfa";
    cache->AddData(query, (uint8_t *) validator_id, sizeof(validator_id));
    cache->AddData(query, (uint8_t *) cpu_features, sizeof(*cpu_features));
    NaClAddCodeIdentity(data, size, metadata, cache, query);
    if (cache->QueryKnownToValidate(query)) {
      cache->DestroyQuery(query);
      return NaClValidationSucceeded;
    }
  }

  if (readonly_text) {
    if (ValidateChunkAMD64(data, size, 0 /*options*/, cpu_features,
                           NaClDfaProcessValidationError,
                           NULL))
      status = NaClValidationSucceeded;
  } else {
    if (ValidateChunkAMD64(data, size, 0 /*options*/, cpu_features,
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


static NaClValidationStatus ValidatorCodeCopy_x86_64(
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
  if (ValidateChunkAMD64(data_new, size, CALL_USER_CALLBACK_ON_EACH_INSTRUCTION,
                         cpu_features, NaClDfaProcessCodeCopyInstruction,
                         &callback_data))
    return NaClValidationSucceeded;
  if (errno == ENOMEM)
    return NaClValidationFailedOutOfMemory;
  return NaClValidationFailed;
}


static Bool ProcessCodeReplacementInstruction(const uint8_t *begin_new,
                                              const uint8_t *end_new,
                                              uint32_t info_new,
                                              void *callback_data) {
  ptrdiff_t existing_minus_new = (ptrdiff_t)callback_data;
  size_t instruction_length = end_new - begin_new;
  const uint8_t *begin_existing = begin_new + existing_minus_new;
  const uint8_t *end_existing = end_new + existing_minus_new;

  /* Sanity check: instruction must be no longer than 17 bytes.  */
  CHECK(instruction_length <= MAX_INSTRUCTION_LENGTH);

  /* Unsupported instruction must have been replaced with HLTs.  */
  if ((info_new & VALIDATION_ERRORS_MASK) == CPUID_UNSUPPORTED_INSTRUCTION)
    return NaClDfaCodeReplacementIsStubouted(begin_existing,
                                             instruction_length);

  /* If we have jump which jumps out of it's range...  */
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

  /* Only some instructions can be modified.  */
  if (!(info_new & MODIFIABLE_INSTRUCTION))
    return FALSE;

  /*
   * In order to understand where the following three cases came from you need
   * to understand that there are three kinds of instructions:
   *  - "normal" x86 instructions
   *  - x86 instructions which [ab]use "immediate" field in the instructions
   *     - 3DNow! x86 instructions use it as an opcode extentions
   *     - four-operand instructions encode fourth register in it
   *  - five-operands instructions encode 2bit immediate and fourth register
   *
   * For the last two cases we need to keep either the whole last byte or at
   * least non-immediate part of it intact.
   *
   * See Figure 1-1 "Instruction Encoding Syntax" in AMDs third volume
   * "General-Purpose and System Instructions" for the information about
   * "normal" x86 instructions and 3DNow! instructions.
   *
   * See Figure 1-1 "Typical Descriptive Synopsis - Extended SSE Instructions"
   * in AMDs fourth volume "128-Bit and 256-Bit Media Instructions" and the
   * "Immediate Byte Usage Unique to the SSE instructions" for the information
   * about four-operand and five-operand instructions.
   */

  /*
   * Instruction with two-bit immediate can only change these two bits and
   * immediate/displacement.
   */
  if ((info_new & IMMEDIATE_2BIT) == IMMEDIATE_2BIT)
    return memcmp(begin_new, begin_existing,
                  instruction_length -
                  INFO_ANYFIELDS_SIZE(info_new) - 1) == 0 &&
           (end_new[-1] & 0xfc) == (end_existing[-1] & 0xfc);

  /* Instruction's last byte is not immediate, thus it must be unchanged.  */
  if (info_new & LAST_BYTE_IS_NOT_IMMEDIATE)
    return memcmp(begin_new, begin_existing,
                  instruction_length -
                  INFO_ANYFIELDS_SIZE(info_new) - 1) == 0 &&
           end_new[-1] == end_existing[-1];

  /*
   * Normal instruction can only change an anyfied: immediate, displacement or
   * relative offset.
   */
  return memcmp(begin_new, begin_existing,
                instruction_length - INFO_ANYFIELDS_SIZE(info_new)) == 0;
}

static NaClValidationStatus ValidatorCodeReplacement_x86_64(
    uintptr_t guest_addr,
    uint8_t *data_existing,
    uint8_t *data_new,
    size_t size,
    const NaClCPUFeatures *f) {
  /* TODO(jfb) Use a safe cast here. */
  NaClCPUFeaturesX86 *cpu_features = (NaClCPUFeaturesX86 *) f;
  UNREFERENCED_PARAMETER(guest_addr);

  if (size & kBundleMask)
    return NaClValidationFailed;
  if (ValidateChunkAMD64(data_new, size, CALL_USER_CALLBACK_ON_EACH_INSTRUCTION,
                         cpu_features, ProcessCodeReplacementInstruction,
                         (void *)(data_existing - data_new)))
    return NaClValidationSucceeded;
  if (errno == ENOMEM)
    return NaClValidationFailedOutOfMemory;
  return NaClValidationFailed;
}

struct IsOnInstBoundaryCallbackData {
  int found_addr;
  const uint8_t* addr;
};

static Bool IsOnInstBoundaryCallback(const uint8_t *begin,
                              const uint8_t *end,
                              uint32_t info,
                              void *callback_data) {
  struct IsOnInstBoundaryCallbackData *data = callback_data;
  UNREFERENCED_PARAMETER(end);

  /*
   * Superinstructions are only reported when the final instruction
   * instruction in the sequence is decoded, therefore if a later
   * address was already marked it must have been inside the superinstruction
   * and needs to be marked invalid.
   */
  if (data->addr > begin && (info & SPECIAL_INSTRUCTION) != 0)
    data->found_addr = FALSE;

  /*
   * For chained instructions in x86-64 the RESTRICTED_REGISTER_USED
   * flag will be set.
   */
  if (begin == data->addr && (info & RESTRICTED_REGISTER_USED) == 0)
    data->found_addr = TRUE;

  /* We should never invalidate the DFA as a whole */
  return TRUE;
}

static NaClValidationStatus IsOnInstBoundary_x86_64(
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

  rc = ValidateChunkAMD64(local_bundle_addr, kBundleSize,
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
  ApplyDfaValidator_x86_64,
  ValidatorCodeCopy_x86_64,
  ValidatorCodeReplacement_x86_64,
  sizeof(NaClCPUFeaturesX86),
  NaClSetAllCPUFeaturesX86,
  NaClGetCurrentCPUFeaturesX86,
  IsOnInstBoundary_x86_64,
};

const struct NaClValidatorInterface *NaClDfaValidatorCreate_x86_64(void) {
  return &validator;
}

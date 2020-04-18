/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVALIDATE_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVALIDATE_H__

/* Defines the API exposed by the Native Client validators. */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/cpu_features/cpu_features.h"


EXTERN_C_BEGIN

struct NaClValidationCache;
struct NaClValidationMetadata;

/* Defines possible validation flags. */
typedef enum NaClValidationFlags {
  NACL_DISABLE_NONTEMPORALS_X86 = 0x1,
  NACL_VALIDATION_FLAGS_MASK_X86 = 0x1,
  NACL_VALIDATION_FLAGS_MASK_ARM = 0x0,
  NACL_VALIDATION_FLAGS_MASK_MIPS = 0x0
} NaClValidationFlags;

/* Defines possible validation status values. */
typedef enum NaClValidationStatus {
  /* The call to the validator succeeded. */
  NaClValidationSucceeded,
  /* The call to the validator failed (Reason unspecified) */
  NaClValidationFailed,
  /* The call to the validator failed, due to not enough memory. */
  NaClValidationFailedOutOfMemory,
  /* The call to the validator failed, due to it not being implemented yet. */
  NaClValidationFailedNotImplemented,
  /* The call to the validator failed, because the CPU is not supported. */
  NaClValidationFailedCpuNotSupported,
  /* The call to the validator failed, due to segment alignment issues. */
  NaClValidationFailedSegmentationIssue
} NaClValidationStatus;

/* Function type for applying a validator, as defined by sel_ldr. That is, run
 * the validator where performance is critical.
 *
 * Parameters are:
 *    guest_addr - The virtual pc to assume is the beginning address of the
 *           code segment. Typically, this is the corresponding address that
 *           will be used by objdump.
 *    data - The contents of the code segment to be validated.
 *    size - The size of the code segment to be validated.
 *    stubout_mode - Whether the validator should stub out disallowed
 *           instructions. This applies the validator silently, stubbing out
 *           instructions that may not validate with a suitable halt
 *           instruction. Note: The return status of NaClValidationSucceeded in
 *           this case does not necessarily imply that all illegal instructions
 *           have been stubbed out. It is the responsibility of the caller to
 *           call the validator a second time to see if the stubbed code is
 *           valid. Typically used as the first step of a stubout tool (either
 *           in sel_ldr or command-line tool).
 *    readonly_text - If code should be considered read-only.
 *    cpu_features - The CPU features to support while validating.
 *    cache - Pointer to NaCl validation cache.
 */
typedef NaClValidationStatus (*NaClValidateFunc)(
    uintptr_t guest_addr,
    uint8_t *data,
    size_t size,
    int stubout_mode,
    uint32_t flags,
    int readonly_text,
    const NaClCPUFeatures *cpu_features,
    const struct NaClValidationMetadata *metadata,
    struct NaClValidationCache *cache);

/* Function type to copy an instruction safely. Returns non-zero on success.
 * Implemented by the Service Runtime.
 */
typedef int (*NaClCopyInstructionFunc)(
    uint8_t *dst,
    uint8_t *src,
    uint8_t sz);

/* Function type for applying a validator to copy code from an existing code
 * segment to a new code segment.
 *
 * Note: Current implementations use the instruction decoders, which may
 * require that the code segment match the Native Client rules.
 *
 * Parameters are:
 *    guest_addr - The virtual pc to assume is the beginning address of the
 *           code segment. Typically, this is the corresponding address that
 *           will be used by objdump.
 *    data_old - The contents of the original code segment.
 *    data_new - The addres of the new code segment for which the original
 *           code segment should be copied into.
 *    size - The size of the passed code segments.
 *    cpu_features - The CPU features to support while validating.
 *    copy_func - Function to perform copying with.
 */
typedef NaClValidationStatus (*NaClCopyCodeFunc)(
    uintptr_t guest_addr,
    uint8_t *data_old,
    uint8_t *data_new,
    size_t size,
    const NaClCPUFeatures *cpu_features,
    NaClCopyInstructionFunc copy_func);

/* Function type for applying a validator on small updates to previously
 * validated code segments.
 *
 * Assumes that instruction sizes are the same. Only allows changes in branches
 * that don't change instruction sizes.
 *
 * Parameters are:
 *    guest_addr - The virtual pc to assume is the beginning address of the
 *           code segment. Typically, this is the corresponding address that
 *           will be used by objdump.
 *    data_old - The contents of the original code segment.
 *    data_new - The contents of the new code segment that should be validated.
 *    size - The size of the passed code segments.
 *    cpu_features - The CPU features to support while validating.
 */
typedef NaClValidationStatus (*NaClValidateCodeReplacementFunc)(
    uintptr_t guest_addr,
    uint8_t *data_old,
    uint8_t *data_new,
    size_t size,
    const NaClCPUFeatures *cpu_features);

typedef void (*NaClCPUFeaturesAllFunc)(NaClCPUFeatures *f);

/* Function type for checking if an address is on the boundary of
 * a single instruction or pseudo-instruction. If it's within a
 * pseudo-instruction it will invalid.
 *
 * Parameters are:
 *    guest_addr - The virtual pc to assume is the beginning address of the
 *           code segment. Typically, this is the corresponding address that
 *           will be used by objdump. Must align to a bundle boundary.
 *    addr - The address of the code to check.
 *    data - The contents of the code segment assumed to be valid.
 *    size - The size of the code segment. Must be a multiple of
             the bundle size.
 *    cpu_features - The CPU features to support while validating.
 */
typedef NaClValidationStatus (*NaClIsOnInstBoundaryFunc)(
    uintptr_t guest_addr,
    uintptr_t addr,
    const uint8_t *data,
    size_t size,
    const NaClCPUFeatures *cpu_features);

/* The full set of validator APIs. */
struct NaClValidatorInterface {
  /* Meta-information for early diagnosis. We assume that at least basic
   * validation is always supported: otherwise NaCl is pretty useless.
   */
  /* Optional stubout_mode. */
  int stubout_mode_implemented;
  /* Optional readonly_text mode. */
  int readonly_text_implemented;
  /* Functions for code replacement.  */
  int code_replacement;
  /* Validation API. */
  NaClValidateFunc Validate;
  NaClCopyCodeFunc CopyCode;
  NaClValidateCodeReplacementFunc ValidateCodeReplacement;
  /* CPU features API, used by validation and caching. */
  size_t CPUFeatureSize;
  /* Set CPU check state fields to all true. */
  NaClCPUFeaturesAllFunc SetAllCPUFeatures;
  /* Get the features for the CPU this code is running on. */
  NaClCPUFeaturesAllFunc GetCurrentCPUFeatures;
  NaClIsOnInstBoundaryFunc IsOnInstBoundary;
};

/* Make a choice of validating functions. */
const struct NaClValidatorInterface *NaClCreateValidator(void);

/* Known Validator API initializers. Private. Do not use outside validator. */
const struct NaClValidatorInterface *NaClValidatorCreate_x86_64(void);
const struct NaClValidatorInterface *NaClValidatorCreate_x86_32(void);
const struct NaClValidatorInterface *NaClDfaValidatorCreate_x86_32(void);
const struct NaClValidatorInterface *NaClDfaValidatorCreate_x86_64(void);
const struct NaClValidatorInterface *NaClValidatorCreateArm(void);
const struct NaClValidatorInterface *NaClValidatorCreateMips(void);

/* Applies the validator, as used in a command-line tool to report issues.
 * Note: This is intentionally separated from ApplyValidator, since it need
 * not be performance critical.
 *
 * Parameters are:
 *    guest_addr - The virtual pc to assume is the beginning address of the
 *           code segment. Typically, this is the corresponding address that
 *           will be used by objdump.
 *    data - The contents of the code segment to be validated.
 *    size - The size of the code segment to be validated.
 *    cpu_features - The CPU features to support while validating.
 */
NaClValidationStatus NACL_SUBARCH_NAME(ApplyValidatorVerbosely,
                                       NACL_BUILD_ARCH,
                                       NACL_BUILD_SUBARCH)(
    uintptr_t guest_addr,
    uint8_t *data,
    size_t size,
    const NaClCPUFeatures *cpu_features);

EXTERN_C_END

#endif  /*  NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVALIDATE_H__ */

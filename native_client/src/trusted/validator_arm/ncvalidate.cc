/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <climits>
#include <limits>
#include <vector>

#include "native_client/src/include/nacl_string.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/cpu_features/arch/arm/cpu_arm.h"
#include "native_client/src/trusted/validator_arm/model.h"
#include "native_client/src/trusted/validator_arm/validator.h"
#include "native_client/src/trusted/validator/validation_cache.h"
#include "native_client/src/trusted/validator/ncvalidate.h"

using nacl_arm_val::SfiValidator;
using nacl_arm_val::CodeSegment;
using nacl_arm_dec::Register;
using nacl_arm_dec::RegisterList;
using std::vector;

static inline bool IsAligned(intptr_t value) {
  return (value & (NACL_BLOCK_SHIFT - 1)) == 0;
}

static inline bool IsAlignedPtr(void *ptr) {
  return IsAligned(reinterpret_cast<intptr_t>(ptr));
}

static const uintptr_t kBytesPerBundle = 1 << NACL_BLOCK_SHIFT;
static const uintptr_t kBytesOfCodeSpace = 1U * 1024 * 1024 * 1024;
static const uintptr_t kBytesOfDataSpace = 1U * 1024 * 1024 * 1024;

// The following checks should have been checked at a higher level, any error
// here is a break of the validator's preconditions. Don't try to recover.
static inline void CheckAddressOverflow(uint8_t *ptr, size_t size) {
  CHECK(uintptr_t(ptr) < std::numeric_limits<uintptr_t>::max() - size);
}

static inline void CheckAddressAlignAndOverflow(uint8_t *ptr, size_t size) {
  CHECK(IsAlignedPtr(ptr));
  CHECK(IsAligned(size));
  CheckAddressOverflow(ptr, size);
}

static NaClValidationStatus ValidatorCopyArm(
    uintptr_t guest_addr,
    uint8_t *data_old,
    uint8_t *data_new,
    size_t size,
    const NaClCPUFeatures *cpu_features,
    NaClCopyInstructionFunc copy_func) {
  /* TODO(jfb) Use a safe cast here. */
  const NaClCPUFeaturesArm *features =
      (const NaClCPUFeaturesArm *) cpu_features;

  CheckAddressAlignAndOverflow((uint8_t *) guest_addr, size);
  CheckAddressOverflow(data_old, size);
  CheckAddressOverflow(data_new, size);

  // guest_addr should always be within 4GB, so static casts should not cause
  // any problems here. They are needed to shut up VC compiler.
  CHECK(guest_addr <= std::numeric_limits<uint32_t>::max());
  CodeSegment dest_code(data_old, static_cast<uint32_t>(guest_addr), size);
  CodeSegment source_code(data_new, static_cast<uint32_t>(guest_addr), size);
  SfiValidator validator(
      kBytesPerBundle,
      kBytesOfCodeSpace,
      kBytesOfDataSpace,
      RegisterList(Register::Tp()),
      RegisterList(Register::Sp()),
      features);

  bool success = validator.CopyCode(source_code, &dest_code, copy_func,
                                    NULL);
  return success ? NaClValidationSucceeded : NaClValidationFailed;
}

static NaClValidationStatus ValidatorCodeReplacementArm(
    uintptr_t guest_addr,
    uint8_t *data_old,
    uint8_t *data_new,
    size_t size,
    const NaClCPUFeatures *cpu_features) {
  /* TODO(jfb) Use a safe cast here. */
  const NaClCPUFeaturesArm *features =
      (const NaClCPUFeaturesArm *) cpu_features;

  CheckAddressAlignAndOverflow((uint8_t *) guest_addr, size);
  CheckAddressOverflow(data_old, size);
  CheckAddressOverflow(data_new, size);

  CHECK(guest_addr <= std::numeric_limits<uint32_t>::max());
  CodeSegment new_code(data_new, static_cast<uint32_t>(guest_addr), size);
  CodeSegment old_code(data_old, static_cast<uint32_t>(guest_addr), size);
  SfiValidator validator(
      kBytesPerBundle,
      kBytesOfCodeSpace,
      kBytesOfDataSpace,
      RegisterList(Register::Tp()),
      RegisterList(Register::Sp()),
      features);

  bool success = validator.ValidateSegmentPair(old_code, new_code, NULL);
  return success ? NaClValidationSucceeded : NaClValidationFailed;
}

EXTERN_C_BEGIN

static int NCValidateSegment(
    uint8_t *mbase,
    uint32_t vbase,
    size_t size,
    const NaClCPUFeaturesArm *features,
    bool *is_position_independent) {

  SfiValidator validator(
      kBytesPerBundle,
      kBytesOfCodeSpace,
      kBytesOfDataSpace,
      RegisterList(Register::Tp()),
      RegisterList(Register::Sp()),
      features);

  vector<CodeSegment> segments;
  segments.push_back(CodeSegment(mbase, vbase, size));

  bool success = validator.validate(segments, NULL);
  *is_position_independent = validator.is_position_independent();
  if (!success) return 2;  // for compatibility with old validator
  return 0;
}

static NaClValidationStatus ApplyValidatorArm(
    uintptr_t guest_addr,
    uint8_t *data,
    size_t size,
    int stubout_mode,
    uint32_t flags,
    int readonly_text,
    const NaClCPUFeatures *cpu_features,
    const struct NaClValidationMetadata *metadata,
    struct NaClValidationCache *cache) {
  // The ARM validator never modifies the text, so this flag can be ignored.
  UNREFERENCED_PARAMETER(readonly_text);
  CHECK((flags & NACL_VALIDATION_FLAGS_MASK_ARM) == 0);
  CheckAddressAlignAndOverflow((uint8_t *) guest_addr, size);
  CheckAddressOverflow(data, size);
  CheckAddressOverflow(data, size);

  if (stubout_mode)
    return NaClValidationFailedNotImplemented;

  CHECK(guest_addr <= std::numeric_limits<uint32_t>::max());

  // These checks are redundant with ones done inside the validator. It is done
  // here so that the cache can memoize the validation result without needing to
  // take guest_addr into account. Note that overflow is checked above. Also
  // note that the sanbox is based at zero so there is no need to check the
  // bottom edge.
  if (guest_addr >= kBytesOfCodeSpace ||
      guest_addr + size > kBytesOfCodeSpace) {
    return NaClValidationFailed;
  }

  /* TODO(jfb) Use a safe cast here. */
  const NaClCPUFeaturesArm *features =
      (const NaClCPUFeaturesArm *) cpu_features;

  /* If the validation caching interface is available, perform a query. */
  void *query = NULL;
  if (cache != NULL && NaClCachingIsInexpensive(cache, metadata))
    query = cache->CreateQuery(cache->handle);
  if (query != NULL) {
    const char validator_id[] = "arm_v2";
    cache->AddData(query, (uint8_t *) validator_id, sizeof(validator_id));
    // The ARM validator is highly parameterizable.  These parameters should not
    // change much in practice, but making them part of the query is a cheap way
    // to be defensive.
    uintptr_t params[] = {
        kBytesPerBundle,
        kBytesOfCodeSpace,
        kBytesOfDataSpace,
        RegisterList(Register::Tp()).bits(),
        RegisterList(Register::Sp()).bits()
    };
    cache->AddData(query, (uint8_t *) params, sizeof(params));
    cache->AddData(query, (uint8_t *) features, sizeof(*features));
    NaClAddCodeIdentity(data, size, metadata, cache, query);
    if (cache->QueryKnownToValidate(query)) {
      cache->DestroyQuery(query);
      return NaClValidationSucceeded;
    }
  }

  bool is_position_independent = false;
  bool ok = NCValidateSegment(data,
                              static_cast<uint32_t>(guest_addr),
                              size,
                              features,
                              &is_position_independent) == 0;

  /* Cache the result if validation succeded. */
  if (query != NULL) {
    if (ok && is_position_independent) {
      cache->SetKnownToValidate(query);
    }
    cache->DestroyQuery(query);
  }

  return ok ? NaClValidationSucceeded : NaClValidationFailed;
}

static NaClValidationStatus IsOnInstBoundaryArm(
    uintptr_t guest_addr,
    uintptr_t addr,
    const uint8_t *data,
    size_t size,
    const NaClCPUFeatures *f) {
  NaClCPUFeaturesArm *cpu_features = (NaClCPUFeaturesArm *) f;
  uint32_t guest_bundle_addr = (addr & ~(kBytesPerBundle - 1));
  const uint8_t *local_bundle_addr = data + (guest_bundle_addr - guest_addr);

  /* Check code range doesn't overflow. */
  CHECK(guest_addr + size > guest_addr);
  CHECK(size % kBytesPerBundle == 0 && size != 0);
  CHECK((uint32_t) (guest_addr & ~(kBytesPerBundle - 1)) == guest_addr);

  /* Check addr is within code boundary. */
  if (addr < guest_addr || addr >= guest_addr + size)
    return NaClValidationFailed;

  SfiValidator validator(
      kBytesPerBundle,
      kBytesOfCodeSpace,
      kBytesOfDataSpace,
      RegisterList(Register::Tp()),
      RegisterList(Register::Sp()),
      cpu_features);

  CodeSegment segment = CodeSegment(
      local_bundle_addr,
      static_cast<uint32_t>(guest_bundle_addr),
      kBytesPerBundle);

  return validator.is_valid_inst_boundary(segment, static_cast<uint32_t>(addr))
    ? NaClValidationSucceeded
    : NaClValidationFailed;
}

static struct NaClValidatorInterface validator = {
  FALSE, /* Optional stubout_mode is not implemented.            */
  FALSE, /* Optional readonly_text mode is not implemented.      */
  TRUE,  /* Optional code replacement functions are implemented. */
  ApplyValidatorArm,
  ValidatorCopyArm,
  ValidatorCodeReplacementArm,
  sizeof(NaClCPUFeaturesArm),
  NaClSetAllCPUFeaturesArm,
  NaClGetCurrentCPUFeaturesArm,
  IsOnInstBoundaryArm,
};

const struct NaClValidatorInterface *NaClValidatorCreateArm() {
  return &validator;
}

/*
 * It should be moved to be part of sel_ldr, not the validator.
 */
int NaClCopyInstruction(uint8_t *dst, uint8_t *src, uint8_t sz) {
  CHECK(sz == nacl_arm_dec::kArm32InstSize / CHAR_BIT);
  *(volatile uint32_t*) dst = *(volatile uint32_t*) src;
  // Don't invalidate i-cache on every instruction update.
  // CPU executing partially updated code doesn't look like a problem,
  // as we know for sure that both old and new instructions are safe,
  // so is their mix (single instruction update is atomic).
  // We just have to make sure that unintended fallthrough doesn't
  // happen, and we don't change position of guard instructions.
  // Problem is that code is mapped for execution at different address
  // that one we use here, and ARM usually use virtually indexed caches,
  // so we couldn't invalidate correctly anyway.
  return 0;
}

EXTERN_C_END

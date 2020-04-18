/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Implement the functions common for ia32 and x86-64 architectures.  */
#include "native_client/src/trusted/validator_ragel/dfa_validate_common.h"

#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/validator_ragel/validator.h"

/* Used as an argument to copy_func when unsupported instruction must be
   replaced with HLTs.  */
static const uint8_t kStubOutMem[MAX_INSTRUCTION_LENGTH] = {
  NACL_HALT_OPCODE, NACL_HALT_OPCODE, NACL_HALT_OPCODE, NACL_HALT_OPCODE,
  NACL_HALT_OPCODE, NACL_HALT_OPCODE, NACL_HALT_OPCODE, NACL_HALT_OPCODE,
  NACL_HALT_OPCODE, NACL_HALT_OPCODE, NACL_HALT_OPCODE, NACL_HALT_OPCODE,
  NACL_HALT_OPCODE, NACL_HALT_OPCODE, NACL_HALT_OPCODE, NACL_HALT_OPCODE,
  NACL_HALT_OPCODE
};

Bool NaClDfaProcessValidationError(const uint8_t *begin, const uint8_t *end,
                                   uint32_t info, void *callback_data) {
  UNREFERENCED_PARAMETER(begin);
  UNREFERENCED_PARAMETER(end);
  UNREFERENCED_PARAMETER(info);
  UNREFERENCED_PARAMETER(callback_data);

  return FALSE;
}

/*
 * Returns whether a validation error should be ignored by
 * RewriteAndRevalidateBundle()'s two validation passes, which
 * validate individual instruction bundles.
 */
static Bool AllowErrorDuringBundleValidation(uint32_t info) {
  if ((info & VALIDATION_ERRORS_MASK) == DIRECT_JUMP_OUT_OF_RANGE) {
    /*
     * This error can occur on valid jumps because we are validating
     * an instruction bundle that is a subset of a code chunk.
     */
    return TRUE;
  }
  return FALSE;
}

#if NACL_BUILD_SUBARCH == 64
static Bool IsREXPrefix(uint8_t byte) {
  return byte >= 0x40 && byte <= 0x4f;
}
#endif

static Bool RewriteNonTemporal(uint8_t *ptr, uint8_t *end, uint32_t info) {
  /*
   * Instruction rewriting.  Note that we only rewrite non-temporal
   * instructions found in nexes and DSOs that are currently found in the
   * Chrome Web Store.  If future nexes use other non-temporal
   * instructions, they will fail validation.
   *
   * We usually only check and rewrite the first few bytes without
   * examining further because this function is only called when the
   * validator tells us that it is an 'unsupported instruction' and there
   * are no other validation failures.
   */
  ptrdiff_t size = end - ptr;

  if (size >= 2 && memcmp(ptr, "\x0f\x18", 2) == 0) {
    /* prefetchnta => nop...nop */
    memset(ptr, 0x90, size);
    return TRUE;
  }

#if NACL_BUILD_SUBARCH == 32
  UNREFERENCED_PARAMETER(info);

  if (size >= 2 && memcmp(ptr, "\x0f\xe7", 2) == 0) {
    /* movntq => movq */
    ptr[1] = 0x7f;
    return TRUE;
  } else if (size >= 2 && memcmp(ptr, "\x0f\x2b", 2) == 0) {
    /* movntps => movaps */
    ptr[1] = 0x29;
    return TRUE;
  } else if (size >= 3 && memcmp(ptr, "\x66\x0f\xe7", 3) == 0) {
    /* movntdq => movdqa */
    ptr[2] = 0x7f;
    return TRUE;
  }
#elif NACL_BUILD_SUBARCH == 64
  if (size >= 3 && IsREXPrefix(ptr[0]) && ptr[1] == 0x0f) {
    uint8_t opcode_byte2 = ptr[2];
    switch (opcode_byte2) {
      case 0xe7:
        /* movntq => movq */
        ptr[2] = 0x7f;
        return TRUE;
      case 0x2b:
        /* movntps => movaps */
        ptr[2] = 0x29;
        return TRUE;
      case 0xc3:
        /* movnti => mov, nop */
        if ((info & RESTRICTED_REGISTER_USED) != 0) {
          /*
           * The rewriting for movnti is special because it changes
           * instruction boundary: movnti is replaced by a mov and a nop so
           * that the total size does not change.  Therefore, special care
           * needs to be taken: if restricted register is used in this
           * instruction, we have to put nop at the end so that the
           * rewritten restricted register consuming instruction follows
           * closely with the restricted register producing instruction (if
           * there is one).
           */
          ptr[1] = 0x89;
          memmove(ptr + 2, ptr + 3, size - 3);
          ptr[size - 1] = 0x90; /* NOP */
        } else {
          /*
           * There are cases where we need to preserve instruction end
           * position, for example, when RIP-relative address is used.
           * Fortunately, RIP-relative addressing cannot use an index
           * register, and therefore RESTRICTED_REGISTER_USED cannot be
           * set.  Therefore, no matter whether RIP-relative addressing is
           * used, as long as restricted register is not used, we are safe
           * to put nop in the beginning and preserve instruction end
           * position.
           */
          ptr[2] = 0x89;
          ptr[1] = ptr[0];
          ptr[0] = 0x90; /* NOP */
        }
        return TRUE;
      case 0x18:
        /* prefetchnta => nop...nop */
        memset(ptr, 0x90, size);
        return TRUE;
      default:
        return FALSE;
    }
  } else if (size >= 4 &&
             ptr[0] == 0x66 &&
             IsREXPrefix(ptr[1]) &&
             memcmp(ptr + 2, "\x0f\xe7", 2) == 0) {
    /* movntdq => movdqa */
    ptr[3] = 0x7f;
    return TRUE;
  }
#else
# error "Unknown architecture"
#endif
  return FALSE;
}

/*
 * First pass of RewriteAndRevalidateBundle(): Rewrite any
 * instructions that need rewriting.
 */
static Bool BundleValidationApplyRewrite(const uint8_t *begin,
                                         const uint8_t *end,
                                         uint32_t info,
                                         void *callback_data) {
  struct StubOutCallbackData *data = callback_data;

  if ((info & VALIDATION_ERRORS_MASK) == CPUID_UNSUPPORTED_INSTRUCTION) {
    /* Stub-out instructions unsupported on this CPU, but valid on other CPUs.*/
    data->did_rewrite = 1;
    memset((uint8_t *) begin, NACL_HALT_OPCODE, end - begin);
    return TRUE;
  }
  if ((info & VALIDATION_ERRORS_MASK) == UNSUPPORTED_INSTRUCTION &&
      (data->flags & NACL_DISABLE_NONTEMPORALS_X86) == 0) {
    if (RewriteNonTemporal((uint8_t *) begin, (uint8_t *) end, info)) {
      data->did_rewrite = 1;
      return TRUE;
    }
    return FALSE;
  }
  return AllowErrorDuringBundleValidation(info);
}

/*
 * Second pass of RewriteAndRevalidateBundle(): Revalidate, checking
 * that no further instruction rewrites are needed.
 */
static Bool BundleValidationCheckAfterRewrite(const uint8_t *begin,
                                              const uint8_t *end,
                                              uint32_t info,
                                              void *callback_data) {
  UNREFERENCED_PARAMETER(begin);
  UNREFERENCED_PARAMETER(end);
  UNREFERENCED_PARAMETER(callback_data);

  return AllowErrorDuringBundleValidation(info);
}

/*
 * As an extra safety check, when the validator modifies an
 * instruction, we want to revalidate the rewritten code.
 *
 * For performance, we don't want to revalidate the whole code chunk,
 * because that would double the overall validation time.  However,
 * it's not practical to revalidate the individual rewritten
 * instruction, because for x86-64 we need to account for previous
 * instructions that can truncate registers to 32 bits (otherwise we'd
 * get an UNRESTRICTED_INDEX_REGISTER error).
 *
 * Therefore, we revalidate the whole bundle instead.  This means we
 * must apply all rewrites to the bundle first.
 *
 * So, when we hit an instruction that needs rewriting, we recursively
 * invoke the validator two times on the instruction bundle:
 *  * First pass: Rewrite all instructions within the bundle that need
 *    rewriting.
 *  * Second pass: Ensure that the rewritten bundle passes the
 *    validator, with no instructions requiring rewrites.
 */
static Bool RewriteAndRevalidateBundle(const uint8_t *instruction_begin,
                                       const uint8_t *instruction_end,
                                       struct StubOutCallbackData *data) {
  uint8_t *bundle_begin;

  /* Find the start of the bundle that this instruction is part of. */
  CHECK(instruction_begin >= data->chunk_begin);
  bundle_begin = data->chunk_begin + ((instruction_begin - data->chunk_begin)
                                      & ~kBundleMask);
  CHECK(instruction_end <= bundle_begin + kBundleSize);
  CHECK(bundle_begin + kBundleSize <= data->chunk_end);

  /* First pass: Rewrite instructions within bundle that need rewriting. */
  if (!data->validate_chunk_func(bundle_begin, kBundleSize, 0 /*options*/,
                                 data->cpu_features,
                                 BundleValidationApplyRewrite,
                                 data)) {
    return FALSE;
  }

  /* Second pass: Revalidate the bundle. */
  return data->validate_chunk_func(bundle_begin, kBundleSize, 0 /*options*/,
                                   data->cpu_features,
                                   BundleValidationCheckAfterRewrite,
                                   data);
}

Bool NaClDfaStubOutUnsupportedInstruction(const uint8_t *begin,
                                          const uint8_t *end,
                                          uint32_t info,
                                          void *callback_data) {
  struct StubOutCallbackData *data = callback_data;
  /* Stub-out instructions unsupported on this CPU, but valid on other CPUs.  */
  if ((info & VALIDATION_ERRORS_MASK) == CPUID_UNSUPPORTED_INSTRUCTION) {
    return RewriteAndRevalidateBundle(begin, end, data);
  }
  if ((info & VALIDATION_ERRORS_MASK) == UNSUPPORTED_INSTRUCTION &&
      (data->flags & NACL_DISABLE_NONTEMPORALS_X86) == 0) {
    return RewriteAndRevalidateBundle(begin, end, data);
  }
  return FALSE;
}

Bool NaClDfaProcessCodeCopyInstruction(const uint8_t *begin_new,
                                       const uint8_t *end_new,
                                       uint32_t info_new,
                                       void *callback_data) {
  struct CodeCopyCallbackData *data = callback_data;
  size_t instruction_length = end_new - begin_new;

  /* Sanity check: instruction must be no longer than 17 bytes.  */
  CHECK(instruction_length <= MAX_INSTRUCTION_LENGTH);

  return data->copy_func(
      (uint8_t *)begin_new + data->existing_minus_new, /* begin_existing */
      (info_new & VALIDATION_ERRORS_MASK) == CPUID_UNSUPPORTED_INSTRUCTION ?
        (uint8_t *)kStubOutMem :
        (uint8_t *)begin_new,
      (uint8_t)instruction_length);
}

Bool NaClDfaCodeReplacementIsStubouted(const uint8_t *begin_existing,
                                       size_t instruction_length) {

  /* Unsupported instruction must have been replaced with HLTs.  */
  if (memcmp(kStubOutMem, begin_existing, instruction_length) == 0)
    return TRUE;
  else
    return FALSE;
}

/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/validator_ragel/validator.h"


/*
 * We don't want to link whole NaCl platform with validator dynamic library,
 * but we need NaClLog (it is used in CHECK), so we provide our own
 * minimal implementation suitable for testing.
 */

void NaClLog(int detail_level, char const *fmt, ...) {
  va_list ap;
  UNREFERENCED_PARAMETER(detail_level);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  if (detail_level == LOG_FATAL)
    exit(1);
}


struct CallbackData {
  const uint8_t *actual_end;
  enum OperandName restricted_register;
  Bool valid;
};


Bool ExtractRestrictedRegisterCallback(
    const uint8_t *begin,
    const uint8_t *end,
    uint32_t info,
    void *data) {

  struct CallbackData *callback_data = data;
  /* UNSUPPORTED_INSTRUCTION indicates validation failure only for pnacl-mode.
   * Since by default we are in non-pnacl-mode, the flag is simply cleared.
   */
  info &= ~UNSUPPORTED_INSTRUCTION;

  if (end <= callback_data->actual_end && (info & VALIDATION_ERRORS_MASK)) {
    callback_data->valid = FALSE;
  }
  if (end == callback_data->actual_end) {
    callback_data->restricted_register =
        (info & RESTRICTED_REGISTER_MASK) >> RESTRICTED_REGISTER_SHIFT;
  }
  if (begin < callback_data->actual_end && callback_data->actual_end < end) {
    /* Instruction crosses actual chunk boundary. */
    callback_data->valid = FALSE;
    return FALSE;
  }
  return TRUE;
}


/*
 * Validate first actual_size bytes of given AMD64 codeblock and return
 * validation result and final value of the restricted register.
 * This function is used in verify_regular_instructions.py test.
 */
VALIDATOR_EXPORT
Bool ValidateAndGetFinalRestrictedRegister(
    const uint8_t codeblock[],
    size_t size,
    size_t actual_size,
    enum OperandName initial_restricted_register,
    const NaClCPUFeaturesX86 *cpu_features,
    enum OperandName *resulting_restricted_register) {

  uint32_t options;
  struct CallbackData callback_data;

  CHECK(size % kBundleSize == 0);
  CHECK(0 < actual_size && actual_size <= size);

  callback_data.actual_end = codeblock + actual_size;
  callback_data.valid = TRUE;

  options =
      PACK_RESTRICTED_REGISTER_INITIAL_VALUE(initial_restricted_register) |
      CALL_USER_CALLBACK_ON_EACH_INSTRUCTION;
  ValidateChunkAMD64(
      codeblock, size,
      options,
      cpu_features,
      ExtractRestrictedRegisterCallback,
      &callback_data);

  *resulting_restricted_register = callback_data.restricted_register;
  return callback_data.valid;
}

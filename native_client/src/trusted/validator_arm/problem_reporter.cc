/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator_arm/problem_reporter.h"

#include <assert.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "native_client/src/include/portability_io.h"

namespace nacl_arm_val {

void ProblemReporter::ReportProblemDiagnostic(nacl_arm_dec::Violation violation,
                                              uint32_t vaddr,
                                              const char* format, ...) {
  // Start by adding address of diagnostic.
  va_list args;
  va_start(args, format);
  VSNPRINTF(buffer, kBufferSize, format, args);
  va_end(args);
  ReportProblemMessage(violation, vaddr, buffer);
}

}  // namespace nacl_arm_val

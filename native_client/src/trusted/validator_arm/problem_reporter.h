/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SOURCE_TRUSTED_VALIDATOR_ARM_PROBLEM_REPORTER_H_
#define NATIVE_CLIENT_SOURCE_TRUSTED_VALIDATOR_ARM_PROBLEM_REPORTER_H_

// Problem reporter utility that converts reported problems into C
// strings.

#include "native_client/src/trusted/validator_arm/validator.h"

namespace nacl_arm_val {

// ProblemSink that converts the (internal) user data into a string
// error message, which is then processed using the (derived) method
// ReportProblemMessage.
class ProblemReporter : public ProblemSink {
 public:
  ProblemReporter() {}
  virtual ~ProblemReporter() {}

  // The following override inherited virtuals.
  virtual void ReportProblemDiagnostic(nacl_arm_dec::Violation violation,
                                       uint32_t vaddr,
                                       const char* format, ...)
               // Note: format is the 4th argument because of implicit this.
               ATTRIBUTE_FORMAT_PRINTF(4, 5);

 protected:
  // Virtual called once the diagnostic message of ReportProblemDiagnostic,
  // or ReportProblemInternal, has been generated.
  virtual void ReportProblemMessage(nacl_arm_dec::Violation violation,
                                    uint32_t vaddr,
                                    const char* message) = 0;

 private:
  // Define a buffer to generate error messages into.
  static const size_t kBufferSize = 256;
  char buffer[kBufferSize];
  NACL_DISALLOW_COPY_AND_ASSIGN(ProblemReporter);
};

}  // namespace

#endif  // NATIVE_CLIENT_SOURCE_TRUSTED_VALIDATOR_ARM_PROBLEM_REPORTER_H_

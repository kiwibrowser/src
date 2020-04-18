/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gtest/gtest.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator/ncvalidate.h"

#define CODE_SIZE 32
#define NOP 0x90
#define MOVNTI_CODE_SIZE 8

// mov %edi,%edi; movnti %rax,0x68(%r15,%rdi,1)
const char* movnti_code = "\x89\xff\x49\x0f\xc3\x44\x3f\x68";

class ValidationDisableNonTemporalsTests : public ::testing::Test {
 protected:
  NaClValidationMetadata *metadata_ptr;
  const struct NaClValidatorInterface *validator;
  NaClCPUFeatures *cpu_features;

  unsigned char code_buffer[CODE_SIZE];

  void SetUp() {
    metadata_ptr = NULL;
    validator = NaClCreateValidator();
    cpu_features = (NaClCPUFeatures *) malloc(validator->CPUFeatureSize);
    EXPECT_NE(cpu_features, (NaClCPUFeatures *) NULL);
    memset(cpu_features, 0, validator->CPUFeatureSize);
    validator->SetAllCPUFeatures(cpu_features);
    memset(code_buffer, NOP, sizeof(code_buffer));
  }

  NaClValidationStatus Validate(uint32_t flags, Bool readonly_text) {
    return validator->Validate(0, code_buffer, 32,
                               FALSE,  /* stubout_mode */
                               flags,
                               readonly_text,
                               cpu_features,
                               metadata_ptr,
                               NULL);
  }

  void TearDown() {
    free(cpu_features);
  }
};

TEST_F(ValidationDisableNonTemporalsTests, NotDisableNonTemporals) {
  memcpy(code_buffer, movnti_code, MOVNTI_CODE_SIZE);
  NaClValidationStatus status = Validate(0, FALSE);
  // If we are not disabling non-temporal instructions, we should rewrite
  // them and return validation success.
  EXPECT_EQ(NaClValidationSucceeded, status);
  // Just make sure code has changed.  The rewriting itself is tested in
  // validation_rewrite_test.cc
  EXPECT_NE(0, memcmp(code_buffer, movnti_code, MOVNTI_CODE_SIZE));
}

TEST_F(ValidationDisableNonTemporalsTests, FailAndNotRewriteWhenReadOnlyText) {
  memcpy(code_buffer, movnti_code, MOVNTI_CODE_SIZE);
  NaClValidationStatus status = Validate(0, TRUE);
  // If we are not disabling non-temporal instructions, we should rewrite
  // them.  However, readonly_text = TRUE would make the text
  // non-modifiable.  In this case, we should return validation failed, and
  // not do rewriting.
  EXPECT_EQ(NaClValidationFailed, status);
  EXPECT_EQ(0, memcmp(code_buffer, movnti_code, MOVNTI_CODE_SIZE));
}

TEST_F(ValidationDisableNonTemporalsTests, DisableNonTemporals) {
  memcpy(code_buffer, movnti_code, MOVNTI_CODE_SIZE);
  NaClValidationStatus status = Validate(NACL_DISABLE_NONTEMPORALS_X86, FALSE);
  // Disable non-temporal instructions.
  EXPECT_EQ(NaClValidationFailed, status);
  // Code should not change.
  EXPECT_EQ(0, memcmp(code_buffer, movnti_code, MOVNTI_CODE_SIZE));
}

int main(int argc, char *argv[]) {
  // The IllegalInst test touches the log mutex deep inside the validator.
  // This causes an SEH exception to be thrown on Windows if the mutex is not
  // initialized.
  // http://code.google.com/p/nativeclient/issues/detail?id=1696
  NaClLogModuleInit();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

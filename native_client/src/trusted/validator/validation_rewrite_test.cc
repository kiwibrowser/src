/*
 * Copyright 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "gtest/gtest.h"

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator/ncvalidate.h"

#define CODE_SIZE 64
#define NOP 0x90

struct TestCode {
  const char *before_template;
  const char *before_template_end;
  const char *after_template;
  const char *after_template_end;
};

#define DECLARE_TEMPLATE(template_name)                                 \
  extern "C" const char template_name[];                                \
  extern "C" const char template_name ## _end[];                        \
  extern "C" const char template_name ## _post_rewrite[];               \
  extern "C" const char template_name ## _post_rewrite_end[];           \
  static const TestCode t_ ## template_name = {                         \
    template_name,                                                      \
    template_name ## _end,                                              \
    template_name ## _post_rewrite,                                     \
    template_name ## _post_rewrite_end,                                 \
  };

DECLARE_TEMPLATE(no_rewrite_code)
#if NACL_BUILD_SUBARCH == 32
DECLARE_TEMPLATE(movntq_code)
DECLARE_TEMPLATE(movntps_code)
DECLARE_TEMPLATE(movntdq_code)
DECLARE_TEMPLATE(prefetchnta_code)
#else
DECLARE_TEMPLATE(off_webstore_movnt_code)
DECLARE_TEMPLATE(prefetchnta_code)
DECLARE_TEMPLATE(prefetchnta_rip_relative_code)
DECLARE_TEMPLATE(movntq_code)
DECLARE_TEMPLATE(movntps_code)
DECLARE_TEMPLATE(movnti_code)
DECLARE_TEMPLATE(movnti_code2)
DECLARE_TEMPLATE(movnti_rip_relative_code)
DECLARE_TEMPLATE(movntdq_code)
DECLARE_TEMPLATE(movntdq_code2)
DECLARE_TEMPLATE(multiple_movnt_code)
DECLARE_TEMPLATE(one_bundle_movnt_code)
DECLARE_TEMPLATE(last_movnti_cross_bundle_by_one)
#endif

class ValidationMovntRewriteTests : public ::testing::Test {
 protected:
  const struct NaClValidatorInterface *validator;
  NaClCPUFeatures *cpu_features;

  unsigned char code_buffer[CODE_SIZE];

  void SetUp() {
    validator = NaClCreateValidator();
    cpu_features = (NaClCPUFeatures *) malloc(validator->CPUFeatureSize);
    EXPECT_NE(cpu_features, (NaClCPUFeatures *) NULL);
    memset(cpu_features, 0, validator->CPUFeatureSize);
    validator->SetAllCPUFeatures(cpu_features);
    memset(code_buffer, NOP, sizeof(code_buffer));
  }

  NaClValidationStatus Validate(uint32_t flags) {
    return validator->Validate(0, code_buffer, CODE_SIZE,
                               FALSE,  /* stubout_mode */
                               flags,
                               FALSE,  /* readonly_test */
                               cpu_features,
                               NULL,
                               NULL);
  }

  void TestTemplate(const TestCode *code,
                    uint32_t flags,
                    NaClValidationStatus expected_status) {
    size_t before_length = code->before_template_end - code->before_template;
    size_t after_length = code->after_template_end - code->after_template;
    EXPECT_EQ(before_length, after_length);
    memcpy(code_buffer, code->before_template, before_length);
    NaClValidationStatus status = Validate(flags);
    EXPECT_EQ(expected_status, status);
    EXPECT_EQ(0, memcmp(code_buffer, code->after_template, after_length));
  }

  void TestRewrite(const TestCode *code) {
    TestTemplate(code, 0, NaClValidationSucceeded);
  }

  void TearDown() {
    free(cpu_features);
  }
};

TEST_F(ValidationMovntRewriteTests, DisableNonTemporalsNoRewrite) {
  TestTemplate(&t_no_rewrite_code, NACL_DISABLE_NONTEMPORALS_X86,
               NaClValidationFailed);
}

#if NACL_BUILD_SUBARCH == 32

TEST_F(ValidationMovntRewriteTests, RewriteMovntq) {
  TestRewrite(&t_movntq_code);
}

TEST_F(ValidationMovntRewriteTests, RewriteMovntps) {
  TestRewrite(&t_movntps_code);
}

TEST_F(ValidationMovntRewriteTests, RewriteMovntdq) {
  TestRewrite(&t_movntdq_code);
}

TEST_F(ValidationMovntRewriteTests, RewritePrefetchnta) {
  TestRewrite(&t_prefetchnta_code);
}

#else

// In this test, the non-temporal write instruction is not found in x86-64
// nexes in the webstore.  Therefore, we will forbid it instead of
// rewriting it.
TEST_F(ValidationMovntRewriteTests, ForbidOffWebStoreMovntNoRewrite) {
  TestTemplate(&t_off_webstore_movnt_code, 0, NaClValidationFailed);
}

TEST_F(ValidationMovntRewriteTests, RewritePrefetchnta) {
  TestRewrite(&t_prefetchnta_code);
}

TEST_F(ValidationMovntRewriteTests, RewritePrefetchntaRipRelative) {
  TestRewrite(&t_prefetchnta_rip_relative_code);
}

TEST_F(ValidationMovntRewriteTests, RewriteMovntq) {
  TestRewrite(&t_movntq_code);
}

TEST_F(ValidationMovntRewriteTests, RewriteMovntps) {
  TestRewrite(&t_movntps_code);
}

TEST_F(ValidationMovntRewriteTests, RewriteMovnti) {
  TestRewrite(&t_movnti_code);
}

TEST_F(ValidationMovntRewriteTests, RewriteMovnti2) {
  TestRewrite(&t_movnti_code2);
}

TEST_F(ValidationMovntRewriteTests, RewriteMovntiRipRelative) {
  TestRewrite(&t_movnti_rip_relative_code);
}

TEST_F(ValidationMovntRewriteTests, RewriteMovntdq) {
  TestRewrite(&t_movntdq_code);
}

TEST_F(ValidationMovntRewriteTests, RewriteMovntdq2) {
  TestRewrite(&t_movntdq_code2);
}

TEST_F(ValidationMovntRewriteTests, RewriteMultipleMovnt) {
  TestRewrite(&t_multiple_movnt_code);
}

TEST_F(ValidationMovntRewriteTests, RewriteOneBundleMovnt) {
  TestRewrite(&t_one_bundle_movnt_code);
}

TEST_F(ValidationMovntRewriteTests, RewriteLastMovntiCrossBundleByOne) {
  TestRewrite(&t_last_movnti_cross_bundle_by_one);
}

#endif

int main(int argc, char *argv[]) {
  NaClLogModuleInit();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

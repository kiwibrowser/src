/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Test for kernel version parsing routines.
 */

#include "gtest/gtest.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/platform_qualify/kernel_version.h"


struct TestData {
  const char *version;
  int ok;
  int num1;
  int num2;
  int num3;
};


class KernelVersionTest : public testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();
};

void KernelVersionTest::SetUp() {
  NaClLogModuleInit();
}

void KernelVersionTest::TearDown() {
  NaClLogModuleFini();
}

TEST_F(KernelVersionTest, ParseKernelVersion) {
  TestData tests[] = {
    { "1.2.3", 1, 1, 2, 3 },
    { "1.2", 1, 1, 2, 0 },
    { "2.6.27", 1, 2, 6, 27 },
    { "2.6.28-smp", 1, 2, 6, 28 },
    { "2.6.30.8-experimental4", 1, 2, 6, 30 },
    { "3.0-rc4", 1, 3, 0, 0 },
    { "zsdjff", 0, 0, 0, 0 },
    { "10.6", 1, 10, 6, 0 },
    { "10.4-intel", 1, 10, 4, 0 },
    { "10-intel", 0, 0, 0, 0 },
  };
  for (size_t i = 0; i < NACL_ARRAY_SIZE(tests); i++) {
    int num1, num2, num3;
    ASSERT_EQ(tests[i].ok, NaClParseKernelVersion(tests[i].version,
                                                  &num1, &num2, &num3));
    if (!tests[i].ok) {
      continue;
    }
    EXPECT_EQ(tests[i].num1, num1);
    EXPECT_EQ(tests[i].num2, num2);
    EXPECT_EQ(tests[i].num3, num3);
  }
}

struct CompareTestData {
  const char* v1;
  const char* v2;
  int ok;
  int res;
};

TEST_F(KernelVersionTest, CompareKernelVersions) {
  CompareTestData tests[] = {
    { "1.2.3", "1.2.4", 1, -1 },
    { "2.6.26.90", "2.6.27", 1, -1 },
    { "3.0", "2.6.27", 1, 1 },
    { "zz", "2.6.27", 0, 0 },
    { "2.6.37.1", "2.6.37.1", 1, 0 },
    { "2.6.37.1", "2.6.37.2", 1, 0 },
    { "10.6.8", "10.8", 1, -1 },
    { "10.8", "10.7", 1, 1 },
  };
  for (size_t i = 0; i < NACL_ARRAY_SIZE(tests); i++) {
    int res;
    ASSERT_EQ(tests[i].ok, NaClCompareKernelVersions(tests[i].v1,
                                                     tests[i].v2,
                                                     &res));
    if (!tests[i].ok) {
      continue;
    }
    EXPECT_EQ(tests[i].res, res);
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

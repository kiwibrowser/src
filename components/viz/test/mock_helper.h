// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_TEST_MOCK_HELPER_H_
#define COMPONENTS_VIZ_TEST_MOCK_HELPER_H_

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest-spi.h"
#include "testing/gtest/include/gtest/gtest.h"

#define EXPECT_MOCK_FAILURE(statement)                            \
  do {                                                            \
    class GTestExpectMockFailureHelper {                          \
     public:                                                      \
      static void Execute() { statement; }                        \
    };                                                            \
    ::testing::TestPartResultArray gtest_failures;                \
    {                                                             \
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter( \
          ::testing::ScopedFakeTestPartResultReporter::           \
              INTERCEPT_ONLY_CURRENT_THREAD,                      \
          &gtest_failures);                                       \
      GTestExpectMockFailureHelper::Execute();                    \
    }                                                             \
    EXPECT_GT(gtest_failures.size(), 0);                          \
  } while (::testing::internal::AlwaysFalse())

#endif  // COMPONENTS_VIZ_TEST_MOCK_HELPER_H_

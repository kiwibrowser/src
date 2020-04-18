// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_UNITTEST_TEST_SUITE_H_
#define CONTENT_PUBLIC_TEST_UNITTEST_TEST_SUITE_H_

#include <memory>

#include "base/macros.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"

#if defined(USE_AURA)
namespace aura {
class AuraTestSuiteSetup;
}
#endif

namespace base {
class TestSuite;
}

namespace content {
class TestBlinkWebUnitTestSupport;

// A special test suite that also initializes WebKit once for all unittests.
// This is useful for two reasons:
// 1. It allows the use of some primitive WebKit data types like WebString.
// 2. Individual unittests should not be initting WebKit on their own, initting
// it here ensures attempts to do so within an individual test will fail.
class UnitTestTestSuite {
 public:
   // Takes ownership of |test_suite|.
  explicit UnitTestTestSuite(base::TestSuite* test_suite);
  ~UnitTestTestSuite();

  int Run();

 private:
  std::unique_ptr<base::TestSuite> test_suite_;

  base::test::ScopedFeatureList feature_list_;

  std::unique_ptr<TestBlinkWebUnitTestSupport> blink_test_support_;
#if defined(USE_AURA)
  std::unique_ptr<aura::AuraTestSuiteSetup> aura_test_suite_setup_;
#endif

  DISALLOW_COPY_AND_ASSIGN(UnitTestTestSuite);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_UNITTEST_TEST_SUITE_H_

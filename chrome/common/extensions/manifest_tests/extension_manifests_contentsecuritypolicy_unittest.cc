// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/common/extensions/manifest_tests/chrome_manifest_test.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/manifest_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace errors = extensions::manifest_errors;
using extensions::ErrorUtils;

class ContentSecurityPolicyManifestTest : public ChromeManifestTest {
};

TEST_F(ContentSecurityPolicyManifestTest, InsecureContentSecurityPolicy) {
  Testcase testcases[] = {
      Testcase(
          "insecure_contentsecuritypolicy_1.json",
          ErrorUtils::FormatErrorMessage(errors::kInvalidCSPInsecureValue,
                                         "http://example.com", "script-src")),
      Testcase("insecure_contentsecuritypolicy_2.json",
               ErrorUtils::FormatErrorMessage(errors::kInvalidCSPInsecureValue,
                                              "'unsafe-inline'", "script-src")),
      Testcase("insecure_contentsecuritypolicy_3.json",
               ErrorUtils::FormatErrorMessage(
                   errors::kInvalidCSPMissingSecureSrc, "object-src"))};
  RunTestcases(testcases, arraysize(testcases), EXPECT_TYPE_WARNING);
}

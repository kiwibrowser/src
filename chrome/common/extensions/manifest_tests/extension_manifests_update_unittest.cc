// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/common/extensions/manifest_tests/chrome_manifest_test.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_url_handlers.h"
#include "testing/gtest/include/gtest/gtest.h"

using extensions::Extension;

namespace errors = extensions::manifest_errors;

class UpdateURLManifestTest : public ChromeManifestTest {
};

TEST_F(UpdateURLManifestTest, UpdateUrls) {
  // Test several valid update urls
  Testcase testcases[] = {
    Testcase("update_url_valid_1.json", extensions::Manifest::INTERNAL,
             Extension::NO_FLAGS),
    Testcase("update_url_valid_2.json", extensions::Manifest::INTERNAL,
             Extension::NO_FLAGS),
    Testcase("update_url_valid_3.json", extensions::Manifest::INTERNAL,
             Extension::NO_FLAGS),
    Testcase("update_url_valid_4.json", extensions::Manifest::INTERNAL,
             Extension::NO_FLAGS)
  };
  RunTestcases(testcases, arraysize(testcases),
               EXPECT_TYPE_SUCCESS);

  // Test some invalid update urls
  Testcase testcases2[] = {
    Testcase("update_url_invalid_1.json", errors::kInvalidUpdateURL,
             extensions::Manifest::INTERNAL, Extension::NO_FLAGS),
    Testcase("update_url_invalid_2.json", errors::kInvalidUpdateURL,
             extensions::Manifest::INTERNAL, Extension::NO_FLAGS),
    Testcase("update_url_invalid_3.json", errors::kInvalidUpdateURL,
             extensions::Manifest::INTERNAL, Extension::NO_FLAGS)
  };
  RunTestcases(testcases2, arraysize(testcases2),
               EXPECT_TYPE_ERROR);
}

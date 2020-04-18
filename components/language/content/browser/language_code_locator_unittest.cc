// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/content/browser/language_code_locator.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace language {

TEST(LanguageCodeLocatorTest, LocatedLanguageOne) {
  LanguageCodeLocator locator;
  std::vector<std::string> expected_langs = {"hi", "mr", "ur"};
  // Random place in Madhya Pradesh, expected langs should be hi;mr;ur.
  const auto& result = locator.GetLanguageCode(23.0, 80.0);
  EXPECT_EQ(expected_langs, result);
}

TEST(LanguageCodeLocatorTest, LocatedLanguageTwo) {
  LanguageCodeLocator locator;
  std::vector<std::string> expected_langs = {"bn"};
  // Random place in Tripura, expected langs should be bn.
  const auto& result = locator.GetLanguageCode(23.7f, 91.7f);
  EXPECT_EQ(expected_langs, result);
}

TEST(LanguageCodeLocatorTest, NotFoundLanguage) {
  LanguageCodeLocator locator;
  std::vector<std::string> expected_langs = {};
  // Random place outside India.
  const auto& result = locator.GetLanguageCode(10.0, 10.0);
  EXPECT_EQ(expected_langs, result);
}

}  // namespace language

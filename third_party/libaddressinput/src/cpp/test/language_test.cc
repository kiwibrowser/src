// Copyright (C) 2014 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "language.h"

#include <string>

#include <gtest/gtest.h>

namespace {

using i18n::addressinput::Language;

struct LanguageTestCase {
  LanguageTestCase(const std::string& input_language_tag,
                   const std::string& expected_language_tag,
                   const std::string& expected_base_language,
                   bool expected_has_latin_script)
      : input_language_tag(input_language_tag),
        expected_language_tag(expected_language_tag),
        expected_base_language(expected_base_language),
        expected_has_latin_script(expected_has_latin_script) {}

  ~LanguageTestCase() {}

  const std::string input_language_tag;
  const std::string expected_language_tag;
  const std::string expected_base_language;
  const bool expected_has_latin_script;
};

class LanguageTest : public testing::TestWithParam<LanguageTestCase> {
 public:
  LanguageTest(const LanguageTest&) = delete;
  LanguageTest& operator=(const LanguageTest&) = delete;

 protected:
  LanguageTest() {}
};

TEST_P(LanguageTest, ExtractedDataIsCorrect) {
  Language language(GetParam().input_language_tag);
  EXPECT_EQ(GetParam().expected_language_tag, language.tag);
  EXPECT_EQ(GetParam().expected_base_language, language.base);
  EXPECT_EQ(GetParam().expected_has_latin_script, language.has_latin_script);
}

INSTANTIATE_TEST_CASE_P(
    LanguageTestCases, LanguageTest,
    testing::Values(
        LanguageTestCase("", "", "", false),
        LanguageTestCase("en", "en", "en", false),
        LanguageTestCase("zh-Latn-CN", "zh-Latn-CN", "zh", true),
        LanguageTestCase("zh-cmn-Latn-CN", "zh-cmn-Latn-CN", "zh", true),
        LanguageTestCase("zh-Hans", "zh-Hans", "zh", false),
        LanguageTestCase("en_GB", "en-GB", "en", false)));

}  // namespace

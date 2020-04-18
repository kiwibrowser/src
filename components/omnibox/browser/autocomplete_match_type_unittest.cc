// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/autocomplete_match_type.h"

#include "base/json/json_reader.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/suggestion_answer.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(AutocompleteMatchTypeTest, AccessibilityLabelHistory) {
  const base::string16& kTestUrl =
      base::UTF8ToUTF16("https://www.chromium.org");
  const base::string16& kTestTitle = base::UTF8ToUTF16("The Chromium Projects");

  // Test plain url.
  AutocompleteMatch match;
  match.type = AutocompleteMatchType::URL_WHAT_YOU_TYPED;
  match.description = kTestTitle;
  EXPECT_EQ(kTestUrl + base::UTF8ToUTF16(", 2 of 9"),
            AutocompleteMatchType::ToAccessibilityLabel(match, kTestUrl, 1, 9));

  // Decorated with title and match type.
  match.type = AutocompleteMatchType::HISTORY_URL;
  EXPECT_EQ(kTestTitle + base::UTF8ToUTF16(" ") + kTestUrl +
                base::UTF8ToUTF16(" location from history, 2 of 3"),
            AutocompleteMatchType::ToAccessibilityLabel(match, kTestUrl, 1, 3));
}

TEST(AutocompleteMatchTypeTest, AccessibilityLabelSearch) {
  const base::string16& kSearch = base::UTF8ToUTF16("gondola");
  const base::string16& kSearchDesc = base::UTF8ToUTF16("Google Search");

  AutocompleteMatch match;
  match.type = AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED;
  match.description = kSearchDesc;
  EXPECT_EQ(kSearch + base::UTF8ToUTF16(" search, 6 of 8"),
            AutocompleteMatchType::ToAccessibilityLabel(match, kSearch, 5, 8));
}

namespace {

std::unique_ptr<SuggestionAnswer> ParseAnswer(const std::string& answer_json) {
  std::unique_ptr<base::Value> value = base::JSONReader::Read(answer_json);
  base::DictionaryValue* dict;
  if (!value || !value->GetAsDictionary(&dict))
    return nullptr;

  return SuggestionAnswer::ParseAnswer(dict);
}

}  // namespace

TEST(AutocompleteMatchTypeTest, AccessibilityLabelAnswer) {
  const base::string16& kSearch = base::UTF8ToUTF16("weather");
  const base::string16& kSearchDesc = base::UTF8ToUTF16("Google Search");

  AutocompleteMatch match;
  match.type = AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED;
  match.description = kSearchDesc;
  std::string answer_json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"t\": \"text\", \"tt\": 8 }] } }, "
      "  { \"il\": { \"t\": [{ \"t\": \"sunny with a chance of hail\", \"tt\": "
      "5 }] } }] }";
  match.answer = ParseAnswer(answer_json);

  EXPECT_EQ(kSearch + base::UTF8ToUTF16(
                          ", answer, sunny with a chance of hail, 4 of 6"),
            AutocompleteMatchType::ToAccessibilityLabel(match, kSearch, 3, 6));
}

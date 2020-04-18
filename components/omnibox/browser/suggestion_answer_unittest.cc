// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/suggestion_answer.h"

#include <algorithm>
#include <memory>

#include "base/json/json_reader.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

std::unique_ptr<SuggestionAnswer> ParseAnswer(const std::string& answer_json) {
  std::unique_ptr<base::Value> value = base::JSONReader::Read(answer_json);
  base::DictionaryValue* dict;
  if (!value || !value->GetAsDictionary(&dict))
    return nullptr;

  return SuggestionAnswer::ParseAnswer(dict);
}

}  // namespace

TEST(SuggestionAnswerTest, DefaultAreEqual) {
  SuggestionAnswer answer1;
  SuggestionAnswer answer2;
  EXPECT_TRUE(answer1.Equals(answer2));
}

TEST(SuggestionAnswerTest, CopiesAreEqual) {
  SuggestionAnswer answer1;
  EXPECT_TRUE(answer1.Equals(SuggestionAnswer(answer1)));

  auto answer2 = std::make_unique<SuggestionAnswer>();
  answer2->set_type(832345);
  EXPECT_TRUE(answer2->Equals(SuggestionAnswer(*answer2)));

  std::string json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"t\": \"text\", \"tt\": 8 }] } }, "
      "  { \"il\": { \"t\": [{ \"t\": \"other text\", \"tt\": 5 }] } } "
      "] }";
  answer2 = ParseAnswer(json);
  ASSERT_TRUE(answer2);
  EXPECT_TRUE(answer2->Equals(SuggestionAnswer(*answer2)));
}

TEST(SuggestionAnswerTest, DifferentValuesAreUnequal) {
  std::string json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"t\": \"text\", \"tt\": 8 }, "
      "                      { \"t\": \"moar text\", \"tt\": 0 }], "
      "              \"i\": { \"d\": \"//example.com/foo.jpg\" } } }, "
      "  { \"il\": { \"t\": [{ \"t\": \"other text\", \"tt\": 5 }], "
      "              \"at\": { \"t\": \"slatfatf\", \"tt\": 42 }, "
      "              \"st\": { \"t\": \"oh hi, Mark\", \"tt\": 729347 } } } "
      "] }";
  std::unique_ptr<SuggestionAnswer> answer1 = ParseAnswer(json);
  ASSERT_TRUE(answer1);

  // Same but with a different answer type.
  std::unique_ptr<SuggestionAnswer> answer2 =
      SuggestionAnswer::copy(answer1.get());
  EXPECT_TRUE(answer1->Equals(*answer2));
  answer2->set_type(44);
  EXPECT_FALSE(answer1->Equals(*answer2));

  // Same but with a different type for one of the text fields.
  answer2 = SuggestionAnswer::copy(answer1.get());
  EXPECT_TRUE(answer1->Equals(*answer2));
  answer2->first_line_.text_fields_[1].type_ = 1;
  EXPECT_FALSE(answer1->Equals(*answer2));

  // Same but with different text for one of the text fields.
  answer2 = SuggestionAnswer::copy(answer1.get());
  EXPECT_TRUE(answer1->Equals(*answer2));
  answer2->first_line_.text_fields_[0].text_ = base::UTF8ToUTF16("some text");
  EXPECT_FALSE(answer1->Equals(*answer2));

  // Same but with a new URL on the second line.
  answer2 = SuggestionAnswer::copy(answer1.get());
  EXPECT_TRUE(answer1->Equals(*answer2));
  answer2->second_line_.image_url_ = GURL("http://foo.com/bar.png");
  EXPECT_FALSE(answer1->Equals(*answer2));

  // Same but with the additional text removed from the second line.
  answer2 = SuggestionAnswer::copy(answer1.get());
  EXPECT_TRUE(answer1->Equals(*answer2));
  answer2->second_line_.additional_text_.reset();
  EXPECT_FALSE(answer1->Equals(*answer2));

  // Same but with the status text removed from the second line.
  answer2 = SuggestionAnswer::copy(answer1.get());
  EXPECT_TRUE(answer1->Equals(*answer2));
  answer2->second_line_.status_text_.reset();
  EXPECT_FALSE(answer1->Equals(*answer2));

  // Same but with the status text removed from the second line of the first
  // answer.
  answer2 = SuggestionAnswer::copy(answer1.get());
  EXPECT_TRUE(answer1->Equals(*answer2));
  answer1->second_line_.status_text_.reset();
  EXPECT_FALSE(answer1->Equals(*answer2));

  // Same but with the additional text removed from the second line of the first
  // answer.
  answer2 = SuggestionAnswer::copy(answer1.get());
  EXPECT_TRUE(answer1->Equals(*answer2));
  answer1->second_line_.additional_text_.reset();
  EXPECT_FALSE(answer1->Equals(*answer2));
}

TEST(SuggestionAnswerTest, EmptyJsonIsInvalid) {
  ASSERT_FALSE(ParseAnswer(""));
}

TEST(SuggestionAnswerTest, MalformedJsonIsInvalid) {
  ASSERT_FALSE(ParseAnswer("} malformed json {"));
}

TEST(SuggestionAnswerTest, TextFieldsRequireBothTextAndType) {
  std::string json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"t\": \"text\" }] } }, "
      "] }";
  ASSERT_FALSE(ParseAnswer(json));

  json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"tt\": 8 }] } }, "
      "] }";
  ASSERT_FALSE(ParseAnswer(json));
}

TEST(SuggestionAnswerTest, ImageLinesMustContainAtLeastOneTextField) {
  std::string json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"t\": \"text\", \"tt\": 8 }, "
      "                      { \"t\": \"moar text\", \"tt\": 0 }], "
      "              \"i\": { \"d\": \"//example.com/foo.jpg\" } } }, "
      "  { \"il\": { \"t\": [], "
      "              \"at\": { \"t\": \"slatfatf\", \"tt\": 42 }, "
      "              \"st\": { \"t\": \"oh hi, Mark\", \"tt\": 729347 } } } "
      "] }";
  ASSERT_FALSE(ParseAnswer(json));
}

TEST(SuggestionAnswerTest, ExactlyTwoLinesRequired) {
  std::string json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"t\": \"text\", \"tt\": 8 }] } }, "
      "] }";
  ASSERT_FALSE(ParseAnswer(json));

  json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"t\": \"text\", \"tt\": 8 }] } }, "
      "  { \"il\": { \"t\": [{ \"t\": \"other text\", \"tt\": 5 }] } } "
      "] }";
  ASSERT_TRUE(ParseAnswer(json));

  json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"t\": \"text\", \"tt\": 8 }] } }, "
      "  { \"il\": { \"t\": [{ \"t\": \"other text\", \"tt\": 5 }] } } "
      "  { \"il\": { \"t\": [{ \"t\": \"yet more text\", \"tt\": 13 }] } } "
      "] }";
  ASSERT_FALSE(ParseAnswer(json));
}

TEST(SuggestionAnswerTest, URLPresent) {
  std::string json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"t\": \"text\", \"tt\": 8 }] } }, "
      "  { \"il\": { \"t\": [{ \"t\": \"other text\", \"tt\": 5 }], "
      "              \"i\": { \"d\": \"\" } } } "
      "] }";
  ASSERT_FALSE(ParseAnswer(json));

  json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"t\": \"text\", \"tt\": 8 }] } }, "
      "  { \"il\": { \"t\": [{ \"t\": \"other text\", \"tt\": 5 }], "
      "              \"i\": { \"d\": \"https://example.com/foo.jpg\" } } } "
      "] }";
  ASSERT_TRUE(ParseAnswer(json));

  json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"t\": \"text\", \"tt\": 8 }] } }, "
      "  { \"il\": { \"t\": [{ \"t\": \"other text\", \"tt\": 5 }], "
      "              \"i\": { \"d\": \"//example.com/foo.jpg\" } } } "
      "] }";
  ASSERT_TRUE(ParseAnswer(json));
}

TEST(SuggestionAnswerTest, ValidPropertyValues) {
  std::string json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"t\": \"text\", \"tt\": 8 }, "
      "                      { \"t\": \"moar text\", \"tt\": 0 }], "
      "              \"i\": { \"d\": \"//example.com/foo.jpg\" } } }, "
      "  { \"il\": { \"t\": [{ \"t\": \"other text\", \"tt\": 5, \"ln\": 3 }], "
      "              \"at\": { \"t\": \"slatfatf\", \"tt\": 42 }, "
      "              \"st\": { \"t\": \"oh hi, Mark\", \"tt\": 729347 } } } "
      "] }";
  std::unique_ptr<SuggestionAnswer> answer = ParseAnswer(json);
  ASSERT_TRUE(answer);
  answer->set_type(420527);
  EXPECT_EQ(420527, answer->type());

  const SuggestionAnswer::ImageLine& first_line = answer->first_line();
  EXPECT_EQ(2U, first_line.text_fields().size());
  EXPECT_EQ(base::UTF8ToUTF16("text"), first_line.text_fields()[0].text());
  EXPECT_EQ(8, first_line.text_fields()[0].type());
  EXPECT_EQ(base::UTF8ToUTF16("moar text"), first_line.text_fields()[1].text());
  EXPECT_EQ(0, first_line.text_fields()[1].type());
  EXPECT_FALSE(first_line.text_fields()[1].has_num_lines());
  EXPECT_EQ(1, first_line.num_text_lines());

  EXPECT_FALSE(first_line.additional_text());
  EXPECT_FALSE(first_line.status_text());

  EXPECT_TRUE(first_line.image_url().is_valid());
  EXPECT_EQ(GURL("https://example.com/foo.jpg"), first_line.image_url());

  const SuggestionAnswer::ImageLine& second_line = answer->second_line();
  EXPECT_EQ(1U, second_line.text_fields().size());
  EXPECT_EQ(
      base::UTF8ToUTF16("other text"), second_line.text_fields()[0].text());
  EXPECT_EQ(5, second_line.text_fields()[0].type());
  EXPECT_TRUE(second_line.text_fields()[0].has_num_lines());
  EXPECT_EQ(3, second_line.text_fields()[0].num_lines());
  EXPECT_EQ(3, second_line.num_text_lines());

  EXPECT_TRUE(second_line.additional_text());
  EXPECT_EQ(
      base::UTF8ToUTF16("slatfatf"), second_line.additional_text()->text());
  EXPECT_EQ(42, second_line.additional_text()->type());

  EXPECT_TRUE(second_line.status_text());
  EXPECT_EQ(
      base::UTF8ToUTF16("oh hi, Mark"), second_line.status_text()->text());
  EXPECT_EQ(729347, second_line.status_text()->type());

  EXPECT_FALSE(second_line.image_url().is_valid());
}

TEST(SuggestionAnswerTest, AddImageURLsTo) {
  SuggestionAnswer::URLs urls;
  std::string json =
      "{ \"l\": ["
      "  { \"il\": { \"t\": [{ \"t\": \"text\", \"tt\": 8 }] } }, "
      "  { \"il\": { \"t\": [{ \"t\": \"other text\", \"tt\": 5 }] } }] }";
  std::unique_ptr<SuggestionAnswer> answer = ParseAnswer(json);
  ASSERT_TRUE(answer);
  answer->AddImageURLsTo(&urls);
  ASSERT_EQ(0U, urls.size());

  {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeature(omnibox::kOmniboxNewAnswerLayout);
    json =
        "{ \"i\": { \"d\": \"https://gstatic.com/foo.png\", \"t\": 3 },"
        "  \"l\" : ["
        "    { \"il\": { \"t\": [{ \"t\": \"some text\", \"tt\": 5 }] } },"
        "    { \"il\": { \"t\": [{ \"t\": \"other text\", \"tt\": 8 }] } }"
        "  ]}";
    answer = ParseAnswer(json);
    ASSERT_TRUE(answer);
    answer->AddImageURLsTo(&urls);
    ASSERT_EQ(1U, urls.size());
    EXPECT_EQ(GURL("https://gstatic.com/foo.png"), urls[0]);
    urls.clear();
  }

  json =
      "{ \"l\" : ["
      "  { \"il\": { \"t\": [{ \"t\": \"some text\", \"tt\": 5 }] } },"
      "  { \"il\": { \"t\": [{ \"t\": \"other text\", \"tt\": 8 }],"
      "              \"i\": { \"d\": \"//gstatic.com/foo.png\", \"t\": 3 }}}]}";
  answer = ParseAnswer(json);
  ASSERT_TRUE(answer);
  answer->AddImageURLsTo(&urls);
  ASSERT_EQ(1U, urls.size());
  EXPECT_EQ(GURL("https://gstatic.com/foo.png"), urls[0]);
  urls.clear();

  json =
      "{ \"i\": { \"d\": \"https://gstatic.com/foo.png\", \"t\": 3 },"
      "  \"l\" : ["
      "    { \"il\": { \"t\": [{ \"t\": \"some text\", \"tt\": 5 }] } },"
      "    { \"il\": { \"t\": [{ \"t\": \"other text\", \"tt\": 8 }],"
      "              \"i\": { \"d\": \"//gstatic.com/bar.png\", \"t\": 3 }}}"
      "  ]}";
  answer = ParseAnswer(json);
  ASSERT_TRUE(answer);
  answer->AddImageURLsTo(&urls);
  ASSERT_EQ(1U, urls.size());
  EXPECT_EQ(GURL("https://gstatic.com/bar.png"), urls[0]);
  urls.clear();

  json =
      "{ \"l\" : ["
      "  { \"il\": { \"t\": [{ \"t\": \"some text\", \"tt\": 5 }],"
      "              \"i\": { \"d\": \"//gstatic.com/foo.png\" } } }, "
      "  { \"il\": { \"t\": [{ \"t\": \"other text\", \"tt\": 8 }],"
      "              \"i\": { \"d\": \"//gstatic.com/bar.jpg\", \"t\": 3 }}}]}";
  answer = ParseAnswer(json);
  ASSERT_TRUE(answer);
  answer->AddImageURLsTo(&urls);
  ASSERT_EQ(1U, urls.size());
  // Note: first_line_.image_url() is not used in practice (so it's ignored).
  EXPECT_EQ(GURL("https://gstatic.com/bar.jpg"), urls[0]);
}

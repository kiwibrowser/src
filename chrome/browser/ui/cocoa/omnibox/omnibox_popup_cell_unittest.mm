// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/omnibox/omnibox_popup_cell.h"

#include <stddef.h>

#include "base/json/json_reader.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "components/omnibox/browser/suggestion_answer.h"
#import "testing/gtest_mac.h"

namespace {

class OmniboxPopupCellTest : public CocoaTest {
 public:
  OmniboxPopupCellTest() {
  }

  void SetUp() override {
    CocoaTest::SetUp();
    control_.reset([[NSControl alloc] initWithFrame:NSMakeRect(0, 0, 200, 20)]);
    [control_ setCell:cell_];
    [[test_window() contentView] addSubview:control_];
  };

 protected:
  base::scoped_nsobject<OmniboxPopupCellData> cellData_;
  base::scoped_nsobject<OmniboxPopupCell> cell_;
  base::scoped_nsobject<NSControl> control_;

 private:
  DISALLOW_COPY_AND_ASSIGN(OmniboxPopupCellTest);
};

TEST_VIEW(OmniboxPopupCellTest, control_);

TEST_F(OmniboxPopupCellTest, Image) {
  AutocompleteMatch match;
  cellData_.reset([[OmniboxPopupCellData alloc]
       initWithMatch:match
               image:[NSImage imageNamed:NSImageNameInfo]
         answerImage:nil
        forDarkTheme:NO]);
  [cell_ setObjectValue:cellData_];
  [control_ display];
}

TEST_F(OmniboxPopupCellTest, Title) {
  AutocompleteMatch match;
  match.contents =
      base::ASCIIToUTF16("The quick brown fox jumps over the lazy dog.");
  cellData_.reset([[OmniboxPopupCellData alloc] initWithMatch:match
                                                        image:nil
                                                  answerImage:nil
                                                 forDarkTheme:NO]);
  [cell_ setObjectValue:cellData_];
  [control_ display];
}

TEST_F(OmniboxPopupCellTest, AnswerStyle) {
  const char* weatherJson =
      "{\"l\": [ {\"il\": {\"t\": [ {"
      "\"t\": \"weather in pari&lt;b&gt;s&lt;/b&gt;\", \"tt\": 8} ]}}, {"
      "\"il\": {\"at\": {\"t\": \"Thu\",\"tt\": 12}, "
      "\"i\": {\"d\": \"//ssl.gstatic.com/onebox/weather/64/cloudy.png\","
      "\"t\": 3}, \"t\": [ {\"t\": \"46\",\"tt\": 1}, {"
      "\"t\": \"°F\",\"tt\": 3} ]}} ]}";
  NSString* finalString = @"46°F Thu";

  std::unique_ptr<base::Value> root(base::JSONReader::Read(weatherJson));
  ASSERT_NE(root, nullptr);
  base::DictionaryValue* dictionary;
  root->GetAsDictionary(&dictionary);
  ASSERT_NE(dictionary, nullptr);
  AutocompleteMatch match;
  match.answer = SuggestionAnswer::ParseAnswer(dictionary);
  EXPECT_TRUE(match.answer);
  cellData_.reset([[OmniboxPopupCellData alloc] initWithMatch:match
                                                        image:nil
                                                  answerImage:nil
                                                 forDarkTheme:NO]);
  EXPECT_NSEQ([[cellData_ description] string], finalString);
  size_t length = [[[cellData_ description] string] length];
  EXPECT_EQ(length, 8UL);
  const NSRange checkValues[] = {{0, 2}, {2, 2}, {4, 4}};
  NSDictionary* lastAttributes = nil;
  for (const NSRange& value : checkValues) {
    NSRange range;
    NSDictionary* currentAttributes =
        [[cellData_ description] attributesAtIndex:value.location
                                    effectiveRange:&range];
    EXPECT_TRUE(NSEqualRanges(value, range));
    EXPECT_FALSE([currentAttributes isEqualToDictionary:lastAttributes]);
    lastAttributes = currentAttributes;
  }
}

TEST_F(OmniboxPopupCellTest, DefinitionAnswerStyle) {
  const char* definitionJson =
      "{\"l\":"
        "["
          "{\"il\":"
            "{\"at\":"
              "{\"t\":\"\u2022 /??s??SH????liz??m/\","
               "\"tt\":8},"
             "\"t\":"
                "["
                  "{\"t\":\"definition of socialism\","
                   "\"tt\":8}"
                "]"
            "}"
          "},"
          "{\"il\":"
            "{\"t\":"
              "["
                "{\"t\":\"a political and economic theory\","
                 "\"tt\":8,"
                 "\"ln\":3}"
              "]"
            "}"
          "}"
        "]"
      "}";
  NSString* finalString = @"a political and economic theory";

  std::unique_ptr<base::Value> root(base::JSONReader::Read(definitionJson));
  ASSERT_NE(root, nullptr);
  base::DictionaryValue* dictionary;
  root->GetAsDictionary(&dictionary);
  ASSERT_NE(dictionary, nullptr);
  AutocompleteMatch match;
  match.answer = SuggestionAnswer::ParseAnswer(dictionary);
  EXPECT_TRUE(match.answer);
  cellData_.reset([[OmniboxPopupCellData alloc] initWithMatch:match
                                                        image:nil
                                                  answerImage:nil
                                                 forDarkTheme:NO]);
  EXPECT_NSEQ([[cellData_ description] string], finalString);
  size_t length = [[[cellData_ description] string] length];
  EXPECT_EQ(length, 31UL);
  const NSRange checkValue = {0, 31};
  NSDictionary* lastAttributes = nil;
  NSRange range;
  NSDictionary* currentAttributes =
      [[cellData_ description] attributesAtIndex:checkValue.location
                                  effectiveRange:&range];
  EXPECT_TRUE(NSEqualRanges(checkValue, range));
  EXPECT_FALSE([currentAttributes isEqualToDictionary:lastAttributes]);
}

}  // namespace

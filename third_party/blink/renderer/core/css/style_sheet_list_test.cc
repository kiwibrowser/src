// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/css/css_rule_list.h"
#include "third_party/blink/renderer/core/css/css_style_sheet.h"
#include "third_party/blink/renderer/core/css/css_style_sheet_init.h"
#include "third_party/blink/renderer/core/css/style_sheet_list.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"

namespace blink {

class StyleSheetListTest : public PageTestBase {
 protected:
  void SetUp() override {
    PageTestBase::SetUp();
    RuntimeEnabledFeatures::SetConstructableStylesheetsEnabled(true);
  }
};

TEST_F(StyleSheetListTest, ConstructorWithoutRuntimeFlagThrowsException) {
  DummyExceptionStateForTesting exception_state;
  RuntimeEnabledFeatures::SetConstructableStylesheetsEnabled(false);
  HeapVector<Member<CSSStyleSheet>> style_sheet_vector;
  EXPECT_EQ(StyleSheetList::Create(style_sheet_vector, exception_state),
            nullptr);
  ASSERT_TRUE(exception_state.HadException());
}

TEST_F(StyleSheetListTest, StyleSheetListConstructionWithEmptyList) {
  DummyExceptionStateForTesting exception_state;
  HeapVector<Member<CSSStyleSheet>> style_sheet_vector;
  StyleSheetList* sheet_list =
      StyleSheetList::Create(style_sheet_vector, exception_state);
  ASSERT_FALSE(exception_state.HadException());
  EXPECT_EQ(sheet_list->length(), 0U);
}

TEST_F(StyleSheetListTest, StyleSheetListConstructionWithNonEmptyList) {
  DummyExceptionStateForTesting exception_state;
  HeapVector<Member<CSSStyleSheet>> style_sheet_vector;
  CSSStyleSheetInit init;
  init.setTitle("Red Sheet");
  CSSStyleSheet* red_style_sheet = CSSStyleSheet::Create(
      GetDocument(), ".red { color: red; }", init, exception_state);
  init.setTitle("Blue Sheet");
  CSSStyleSheet* blue_style_sheet = CSSStyleSheet::Create(
      GetDocument(), ".blue { color: blue; }", init, exception_state);
  style_sheet_vector.push_back(red_style_sheet);
  style_sheet_vector.push_back(blue_style_sheet);

  StyleSheetList* sheet_list =
      StyleSheetList::Create(style_sheet_vector, exception_state);
  ASSERT_FALSE(exception_state.HadException());
  EXPECT_EQ(sheet_list->length(), 2U);
  EXPECT_EQ(sheet_list->item(0), red_style_sheet);
  EXPECT_EQ(sheet_list->item(1), blue_style_sheet);
}

TEST_F(StyleSheetListTest, GetNamedItemNoTreeScope) {
  StyleSheetList* list = StyleSheetList::Create();
  EXPECT_FALSE(list->GetNamedItem("id"));
}

}  // namespace blink

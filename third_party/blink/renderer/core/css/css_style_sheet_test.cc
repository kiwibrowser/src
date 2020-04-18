// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/media_list_or_string.h"
#include "third_party/blink/renderer/core/css/css_rule_list.h"
#include "third_party/blink/renderer/core/css/css_style_sheet.h"
#include "third_party/blink/renderer/core/css/css_style_sheet_init.h"
#include "third_party/blink/renderer/core/css/media_list.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"

namespace blink {

class CSSStyleSheetTest : public PageTestBase {
 protected:
  void SetUp() override {
    PageTestBase::SetUp();
    RuntimeEnabledFeatures::SetConstructableStylesheetsEnabled(true);
  }
};

TEST_F(CSSStyleSheetTest, ConstructorWithoutRuntimeFlagThrowsException) {
  DummyExceptionStateForTesting exception_state;
  RuntimeEnabledFeatures::SetConstructableStylesheetsEnabled(false);
  EXPECT_EQ(CSSStyleSheet::Create(GetDocument(), "", CSSStyleSheetInit(),
                                  exception_state),
            nullptr);
  ASSERT_TRUE(exception_state.HadException());
}

TEST_F(CSSStyleSheetTest,
       CSSStyleSheetConstructionWithEmptyCSSStyleSheetInitAndText) {
  DummyExceptionStateForTesting exception_state;
  CSSStyleSheet* sheet = CSSStyleSheet::Create(
      GetDocument(), "", CSSStyleSheetInit(), exception_state);
  ASSERT_FALSE(exception_state.HadException());
  EXPECT_TRUE(sheet->href().IsNull());
  EXPECT_EQ(sheet->parentStyleSheet(), nullptr);
  EXPECT_EQ(sheet->ownerNode(), nullptr);
  EXPECT_EQ(sheet->ownerRule(), nullptr);
  EXPECT_EQ(sheet->media()->length(), 0U);
  EXPECT_EQ(sheet->title(), StringImpl::empty_);
  EXPECT_FALSE(sheet->AlternateFromConstructor());
  EXPECT_FALSE(sheet->disabled());
  EXPECT_EQ(sheet->cssRules(exception_state)->length(), 0U);
  ASSERT_FALSE(exception_state.HadException());
}

TEST_F(CSSStyleSheetTest,
       CSSStyleSheetConstructionWithoutEmptyCSSStyleSheetInitAndText) {
  DummyExceptionStateForTesting exception_state;
  String styleText[2] = {".red { color: red; }",
                         ".red + span + span { color: red; }"};
  CSSStyleSheetInit init;
  init.setMedia(MediaListOrString::FromString("screen, print"));
  init.setTitle("test");
  init.setAlternate(true);
  init.setDisabled(true);
  CSSStyleSheet* sheet = CSSStyleSheet::Create(
      GetDocument(), styleText[0] + styleText[1], init, exception_state);
  ASSERT_FALSE(exception_state.HadException());
  EXPECT_TRUE(sheet->href().IsNull());
  EXPECT_EQ(sheet->parentStyleSheet(), nullptr);
  EXPECT_EQ(sheet->ownerNode(), nullptr);
  EXPECT_EQ(sheet->ownerRule(), nullptr);
  EXPECT_EQ(sheet->media()->length(), 2U);
  EXPECT_EQ(sheet->media()->mediaText(), init.media().GetAsString());
  EXPECT_EQ(sheet->title(), init.title());
  EXPECT_TRUE(sheet->AlternateFromConstructor());
  EXPECT_TRUE(sheet->disabled());
  EXPECT_EQ(sheet->cssRules(exception_state)->length(), 2U);
  EXPECT_EQ(sheet->cssRules(exception_state)->item(0)->cssText(), styleText[0]);
  EXPECT_EQ(sheet->cssRules(exception_state)->item(1)->cssText(), styleText[1]);
  ASSERT_FALSE(exception_state.HadException());
}

}  // namespace blink

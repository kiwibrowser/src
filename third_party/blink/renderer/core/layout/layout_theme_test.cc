// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/layout_theme.h"

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/node_computed_style.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/page/focus_controller.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/platform/graphics/color.h"

namespace blink {

class LayoutThemeTest : public PageTestBase {
 protected:
  void SetHtmlInnerHTML(const char* html_content);
};

void LayoutThemeTest::SetHtmlInnerHTML(const char* html_content) {
  GetDocument().documentElement()->SetInnerHTMLFromString(
      String::FromUTF8(html_content));
  GetDocument().View()->UpdateAllLifecyclePhases();
}

inline Color OutlineColor(Element* element) {
  return element->GetComputedStyle()->VisitedDependentColor(
      GetCSSPropertyOutlineColor());
}

inline EBorderStyle OutlineStyle(Element* element) {
  return element->GetComputedStyle()->OutlineStyle();
}

TEST_F(LayoutThemeTest, ChangeFocusRingColor) {
  SetHtmlInnerHTML("<span id=span tabIndex=0>Span</span>");

  Element* span = GetDocument().getElementById(AtomicString("span"));
  EXPECT_NE(nullptr, span);
  EXPECT_NE(nullptr, span->GetLayoutObject());

  Color custom_color = MakeRGB(123, 145, 167);

  // Checking unfocused style.
  EXPECT_EQ(EBorderStyle::kNone, OutlineStyle(span));
  EXPECT_NE(custom_color, OutlineColor(span));

  // Do focus.
  GetDocument().GetPage()->GetFocusController().SetActive(true);
  GetDocument().GetPage()->GetFocusController().SetFocused(true);
  span->focus();
  GetDocument().View()->UpdateAllLifecyclePhases();

  // Checking focused style.
  EXPECT_NE(EBorderStyle::kNone, OutlineStyle(span));
  EXPECT_NE(custom_color, OutlineColor(span));

  // Change focus ring color.
  LayoutTheme::GetTheme().SetCustomFocusRingColor(custom_color);
  Page::PlatformColorsChanged();
  GetDocument().View()->UpdateAllLifecyclePhases();

  // Check that the focus ring color is updated.
  EXPECT_NE(EBorderStyle::kNone, OutlineStyle(span));
  EXPECT_EQ(custom_color, OutlineColor(span));
}

}  // namespace blink

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/ax_object_cache.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/modules/accessibility/testing/accessibility_test.h"

namespace blink {

// TODO(nektar): Break test up into multiple tests.
TEST_F(AccessibilityTest, IsARIAWidget) {
  String test_content =
      "<body>"
      "<span id=\"plain\">plain</span><br>"
      "<span id=\"button\" role=\"button\">button</span><br>"
      "<span id=\"button-parent\" "
      "role=\"button\"><span>button-parent</span></span><br>"
      "<span id=\"button-caps\" role=\"BUTTON\">button-caps</span><br>"
      "<span id=\"button-second\" role=\"another-role "
      "button\">button-second</span><br>"
      "<span id=\"aria-bogus\" aria-bogus=\"bogus\">aria-bogus</span><br>"
      "<span id=\"aria-selected\" aria-selected>aria-selected</span><br>"
      "<span id=\"haspopup\" "
      "aria-haspopup=\"true\">aria-haspopup-true</span><br>"
      "<div id=\"focusable\" tabindex=\"1\">focusable</div><br>"
      "<div tabindex=\"2\"><div "
      "id=\"focusable-parent\">focusable-parent</div></div><br>"
      "</body>";

  SetBodyInnerHTML(test_content);
  Element* root(GetDocument().documentElement());
  EXPECT_FALSE(AXObjectCache::IsInsideFocusableElementOrARIAWidget(
      *root->getElementById("plain")));
  EXPECT_TRUE(AXObjectCache::IsInsideFocusableElementOrARIAWidget(
      *root->getElementById("button")));
  EXPECT_TRUE(AXObjectCache::IsInsideFocusableElementOrARIAWidget(
      *root->getElementById("button-parent")));
  EXPECT_TRUE(AXObjectCache::IsInsideFocusableElementOrARIAWidget(
      *root->getElementById("button-caps")));
  EXPECT_TRUE(AXObjectCache::IsInsideFocusableElementOrARIAWidget(
      *root->getElementById("button-second")));
  EXPECT_FALSE(AXObjectCache::IsInsideFocusableElementOrARIAWidget(
      *root->getElementById("aria-bogus")));
  EXPECT_TRUE(AXObjectCache::IsInsideFocusableElementOrARIAWidget(
      *root->getElementById("aria-selected")));
  EXPECT_TRUE(AXObjectCache::IsInsideFocusableElementOrARIAWidget(
      *root->getElementById("haspopup")));
  EXPECT_TRUE(AXObjectCache::IsInsideFocusableElementOrARIAWidget(
      *root->getElementById("focusable")));
  EXPECT_TRUE(AXObjectCache::IsInsideFocusableElementOrARIAWidget(
      *root->getElementById("focusable-parent")));
}

}  // namespace blink

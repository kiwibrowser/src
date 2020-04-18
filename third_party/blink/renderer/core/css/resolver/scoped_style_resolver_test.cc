// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/resolver/scoped_style_resolver.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/dom/shadow_root_init.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"

namespace blink {

class ScopedStyleResolverTest : public PageTestBase {
 protected:
  StyleEngine& GetStyleEngine() { return GetDocument().GetStyleEngine(); }
};

TEST_F(ScopedStyleResolverTest, HasSameStylesNullNull) {
  EXPECT_TRUE(ScopedStyleResolver::HaveSameStyles(nullptr, nullptr));
}

TEST_F(ScopedStyleResolverTest, HasSameStylesNullEmpty) {
  ScopedStyleResolver& resolver = GetDocument().EnsureScopedStyleResolver();
  EXPECT_TRUE(ScopedStyleResolver::HaveSameStyles(nullptr, &resolver));
  EXPECT_TRUE(ScopedStyleResolver::HaveSameStyles(&resolver, nullptr));
}

TEST_F(ScopedStyleResolverTest, HasSameStylesEmptyEmpty) {
  ScopedStyleResolver& resolver = GetDocument().EnsureScopedStyleResolver();
  EXPECT_TRUE(ScopedStyleResolver::HaveSameStyles(&resolver, &resolver));
}

TEST_F(ScopedStyleResolverTest, HasSameStylesNonEmpty) {
  GetDocument().body()->SetInnerHTMLFromString(
      "<div id=host1></div><div id=host2></div>");
  Element* host1 = GetDocument().getElementById("host1");
  Element* host2 = GetDocument().getElementById("host2");
  ASSERT_TRUE(host1);
  ASSERT_TRUE(host2);
  ShadowRoot& root1 = host1->AttachShadowRootInternal(ShadowRootType::kOpen);
  ShadowRoot& root2 = host2->AttachShadowRootInternal(ShadowRootType::kOpen);
  root1.SetInnerHTMLFromString("<style>::slotted(#dummy){color:pink}</style>");
  root2.SetInnerHTMLFromString("<style>::slotted(#dummy){color:pink}</style>");
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_TRUE(ScopedStyleResolver::HaveSameStyles(
      &root1.EnsureScopedStyleResolver(), &root2.EnsureScopedStyleResolver()));
}

TEST_F(ScopedStyleResolverTest, HasSameStylesDifferentSheetCount) {
  GetDocument().body()->SetInnerHTMLFromString(
      "<div id=host1></div><div id=host2></div>");
  Element* host1 = GetDocument().getElementById("host1");
  Element* host2 = GetDocument().getElementById("host2");
  ASSERT_TRUE(host1);
  ASSERT_TRUE(host2);
  ShadowRoot& root1 = host1->AttachShadowRootInternal(ShadowRootType::kOpen);
  ShadowRoot& root2 = host2->AttachShadowRootInternal(ShadowRootType::kOpen);
  root1.SetInnerHTMLFromString(
      "<style>::slotted(#dummy){color:pink}</style><style>div{}</style>");
  root2.SetInnerHTMLFromString("<style>::slotted(#dummy){color:pink}</style>");
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(ScopedStyleResolver::HaveSameStyles(
      &root1.EnsureScopedStyleResolver(), &root2.EnsureScopedStyleResolver()));
}

TEST_F(ScopedStyleResolverTest, HasSameStylesCacheMiss) {
  GetDocument().body()->SetInnerHTMLFromString(
      "<div id=host1></div><div id=host2></div>");
  Element* host1 = GetDocument().getElementById("host1");
  Element* host2 = GetDocument().getElementById("host2");
  ASSERT_TRUE(host1);
  ASSERT_TRUE(host2);
  ShadowRoot& root1 = host1->AttachShadowRootInternal(ShadowRootType::kOpen);
  ShadowRoot& root2 = host2->AttachShadowRootInternal(ShadowRootType::kOpen);
  // Style equality is detected when StyleSheetContents is shared. That is only
  // the case when the source text is the same. The comparison will fail when
  // adding an extra space to one of the sheets.
  root1.SetInnerHTMLFromString("<style>::slotted(#dummy){color:pink}</style>");
  root2.SetInnerHTMLFromString("<style>::slotted(#dummy){ color:pink}</style>");
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(ScopedStyleResolver::HaveSameStyles(
      &root1.EnsureScopedStyleResolver(), &root2.EnsureScopedStyleResolver()));
}

}  // namespace blink

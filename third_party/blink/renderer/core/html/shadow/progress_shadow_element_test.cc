// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/shadow/progress_shadow_element.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/node_computed_style.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/html/html_progress_element.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"

namespace blink {

class ProgressShadowElementTest : public testing::Test {
 protected:
  void SetUp() final {
    dummy_page_holder_ = DummyPageHolder::Create(IntSize(800, 600));
  }
  Document& GetDocument() { return dummy_page_holder_->GetDocument(); }

 private:
  std::unique_ptr<DummyPageHolder> dummy_page_holder_;
};

TEST_F(ProgressShadowElementTest, LayoutObjectIsNeeded) {
  GetDocument().body()->SetInnerHTMLFromString(R"HTML(
    <progress id='prog' style='-webkit-appearance:none' />
  )HTML");

  HTMLProgressElement* progress =
      ToHTMLProgressElement(GetDocument().getElementById("prog"));
  ASSERT_TRUE(progress);

  Element* shadow_element = ToElement(progress->GetShadowRoot()->firstChild());
  ASSERT_TRUE(shadow_element);

  GetDocument().View()->UpdateAllLifecyclePhases();

  progress->LazyReattachIfAttached();
  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);
  GetDocument().documentElement()->RecalcStyle(kForce);
  EXPECT_TRUE(shadow_element->GetNonAttachedStyle());

  scoped_refptr<ComputedStyle> style = shadow_element->StyleForLayoutObject();
  EXPECT_TRUE(shadow_element->LayoutObjectIsNeeded(*style));
}

}  // namespace blink

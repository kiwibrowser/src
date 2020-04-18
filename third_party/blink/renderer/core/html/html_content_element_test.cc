// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/html_content_element.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"

namespace blink {

class HTMLContentElementTest : public testing::Test {
 protected:
  void SetUp() final {
    dummy_page_holder_ = DummyPageHolder::Create(IntSize(800, 600));
  }
  Document& GetDocument() { return dummy_page_holder_->GetDocument(); }

 private:
  std::unique_ptr<DummyPageHolder> dummy_page_holder_;
};

TEST_F(HTMLContentElementTest, FallbackRecalcForReattach) {
  GetDocument().body()->SetInnerHTMLFromString(R"HTML(
    <div id='host'></div>
  )HTML");

  Element* host = GetDocument().getElementById("host");
  ShadowRoot& root = host->CreateV0ShadowRootForTesting();
  GetDocument().View()->UpdateAllLifecyclePhases();

  auto* content = GetDocument().CreateRawElement(HTMLNames::contentTag);
  auto* fallback = GetDocument().CreateRawElement(HTMLNames::spanTag);
  content->AppendChild(fallback);
  root.AppendChild(content);

  GetDocument().UpdateDistributionForLegacyDistributedNodes();
  GetDocument().Lifecycle().AdvanceTo(DocumentLifecycle::kInStyleRecalc);
  GetDocument().documentElement()->RecalcStyle(kNoChange);

  EXPECT_TRUE(fallback->GetNonAttachedStyle());
}

}  // namespace blink

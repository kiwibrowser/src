// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/html_meta_element.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

class HTMLMetaElementTest : public PageTestBase {
 public:
  void SetUp() override {
    PageTestBase::SetUp();

    RuntimeEnabledFeatures::SetDisplayCutoutViewportFitEnabled(true);
    GetDocument().GetSettings()->SetViewportMetaEnabled(true);
  }

  ViewportDescription::ViewportFit LoadTestPageAndReturnViewportFit(
      const String& value) {
    LoadTestPageWithViewportFitValue(value);
    return GetDocument().GetViewportDescription().GetViewportFit();
  }

 private:
  void LoadTestPageWithViewportFitValue(const String& value) {
    GetDocument().documentElement()->SetInnerHTMLFromString(
        "<head>"
        "<meta name='viewport' content='viewport-fit=" +
        value +
        "'>"
        "</head>");
  }
};

TEST_F(HTMLMetaElementTest, ViewportFit_Auto) {
  EXPECT_EQ(ViewportDescription::ViewportFit::kAuto,
            LoadTestPageAndReturnViewportFit("auto"));
}

TEST_F(HTMLMetaElementTest, ViewportFit_Contain) {
  EXPECT_EQ(ViewportDescription::ViewportFit::kContain,
            LoadTestPageAndReturnViewportFit("contain"));
}

TEST_F(HTMLMetaElementTest, ViewportFit_Cover) {
  EXPECT_EQ(ViewportDescription::ViewportFit::kCover,
            LoadTestPageAndReturnViewportFit("cover"));
}

TEST_F(HTMLMetaElementTest, ViewportFit_Invalid) {
  EXPECT_EQ(ViewportDescription::ViewportFit::kAuto,
            LoadTestPageAndReturnViewportFit("invalid"));
}

}  // namespace blink

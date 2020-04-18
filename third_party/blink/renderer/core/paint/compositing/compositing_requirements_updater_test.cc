// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/compositing/compositing_requirements_updater.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"

namespace blink {

class CompositingRequirementsUpdaterTest : public RenderingTest {
 public:
  CompositingRequirementsUpdaterTest()
      : RenderingTest(SingleChildLocalFrameClient::Create()) {}

  void SetUp() final;
};

void CompositingRequirementsUpdaterTest::SetUp() {
  RenderingTest::SetUp();
  EnableCompositing();
}

TEST_F(CompositingRequirementsUpdaterTest, FixedPosOverlap) {
  SetBodyInnerHTML(R"HTML(
    <div style="position: relative; width: 500px; height: 300px;
        will-change: transform"></div>
    <div id=fixed style="position: fixed; width: 500px; height: 300px;
        top: 300px"></div>
    <div style="width: 200px; height: 3000px"></div>
  )HTML");

  LayoutBoxModelObject* fixed =
      ToLayoutBoxModelObject(GetLayoutObjectByElementId("fixed"));

  EXPECT_EQ(
      CompositingReason::kOverlap | CompositingReason::kSquashingDisallowed,
      fixed->Layer()->GetCompositingReasons());

  GetDocument().View()->LayoutViewportScrollableArea()->ScrollBy(
      ScrollOffset(0, 100), kUserScroll);
  GetDocument().View()->UpdateAllLifecyclePhases();

  // No longer overlaps the first div.
  EXPECT_EQ(CompositingReason::kNone, fixed->Layer()->GetCompositingReasons());
}

}  // namespace blink

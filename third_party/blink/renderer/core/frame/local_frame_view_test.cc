// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/local_frame_view.h"

#include <memory>

#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_property_tree_printer.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_artifact.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

using testing::_;
using testing::AnyNumber;

namespace blink {
namespace {

class AnimationMockChromeClient : public EmptyChromeClient {
 public:
  AnimationMockChromeClient() : has_scheduled_animation_(false) {}

  // ChromeClient
  MOCK_METHOD2(AttachRootGraphicsLayer,
               void(GraphicsLayer*, LocalFrame* localRoot));
  MOCK_METHOD3(MockSetToolTip, void(LocalFrame*, const String&, TextDirection));
  void SetToolTip(LocalFrame& frame,
                  const String& tooltip_text,
                  TextDirection dir) override {
    MockSetToolTip(&frame, tooltip_text, dir);
  }

  void ScheduleAnimation(const LocalFrameView*) override {
    has_scheduled_animation_ = true;
  }
  bool has_scheduled_animation_;
};

class LocalFrameViewTest : public RenderingTest {
 protected:
  LocalFrameViewTest()
      : RenderingTest(SingleChildLocalFrameClient::Create()),
        chrome_client_(new AnimationMockChromeClient) {
    EXPECT_CALL(GetAnimationMockChromeClient(), AttachRootGraphicsLayer(_, _))
        .Times(AnyNumber());
  }

  ~LocalFrameViewTest() override {
    testing::Mock::VerifyAndClearExpectations(&GetAnimationMockChromeClient());
  }

  ChromeClient& GetChromeClient() const override { return *chrome_client_; }

  void SetUp() override {
    RenderingTest::SetUp();
    EnableCompositing();
  }

  AnimationMockChromeClient& GetAnimationMockChromeClient() const {
    return *chrome_client_;
  }

 private:
  Persistent<AnimationMockChromeClient> chrome_client_;
};

TEST_F(LocalFrameViewTest, SetPaintInvalidationDuringUpdateAllLifecyclePhases) {
  SetBodyInnerHTML("<div id='a' style='color: blue'>A</div>");
  GetDocument().getElementById("a")->setAttribute(HTMLNames::styleAttr,
                                                  "color: green");
  GetAnimationMockChromeClient().has_scheduled_animation_ = false;
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(GetAnimationMockChromeClient().has_scheduled_animation_);
}

TEST_F(LocalFrameViewTest, SetPaintInvalidationOutOfUpdateAllLifecyclePhases) {
  SetBodyInnerHTML("<div id='a' style='color: blue'>A</div>");
  GetAnimationMockChromeClient().has_scheduled_animation_ = false;
  GetDocument()
      .getElementById("a")
      ->GetLayoutObject()
      ->SetShouldDoFullPaintInvalidation();
  EXPECT_TRUE(GetAnimationMockChromeClient().has_scheduled_animation_);
  GetAnimationMockChromeClient().has_scheduled_animation_ = false;
  GetDocument()
      .getElementById("a")
      ->GetLayoutObject()
      ->SetShouldDoFullPaintInvalidation();
  EXPECT_TRUE(GetAnimationMockChromeClient().has_scheduled_animation_);
  GetAnimationMockChromeClient().has_scheduled_animation_ = false;
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(GetAnimationMockChromeClient().has_scheduled_animation_);
}

// If we don't hide the tooltip on scroll, it can negatively impact scrolling
// performance. See crbug.com/586852 for details.
TEST_F(LocalFrameViewTest, HideTooltipWhenScrollPositionChanges) {
  SetBodyInnerHTML("<div style='width:1000px;height:1000px'></div>");

  EXPECT_CALL(GetAnimationMockChromeClient(),
              MockSetToolTip(GetDocument().GetFrame(), String(), _));
  GetDocument().View()->LayoutViewportScrollableArea()->SetScrollOffset(
      ScrollOffset(1, 1), kUserScroll);

  // Programmatic scrolling should not dismiss the tooltip, so setToolTip
  // should not be called for this invocation.
  EXPECT_CALL(GetAnimationMockChromeClient(),
              MockSetToolTip(GetDocument().GetFrame(), String(), _))
      .Times(0);
  GetDocument().View()->LayoutViewportScrollableArea()->SetScrollOffset(
      ScrollOffset(2, 2), kProgrammaticScroll);
}

// NoOverflowInIncrementVisuallyNonEmptyPixelCount tests fail if the number of
// pixels is calculated in 32-bit integer, because 65536 * 65536 would become 0
// if it was calculated in 32-bit and thus it would be considered as empty.
TEST_F(LocalFrameViewTest, NoOverflowInIncrementVisuallyNonEmptyPixelCount) {
  EXPECT_FALSE(GetDocument().View()->IsVisuallyNonEmpty());
  GetDocument().View()->IncrementVisuallyNonEmptyPixelCount(
      IntSize(65536, 65536));
  EXPECT_TRUE(GetDocument().View()->IsVisuallyNonEmpty());
}

// This test addresses http://crbug.com/696173, in which a call to
// LocalFrameView::UpdateLayersAndCompositingAfterScrollIfNeeded during layout
// caused a crash as the code was incorrectly assuming that the ancestor
// overflow layer would always be valid.
TEST_F(LocalFrameViewTest,
       ViewportConstrainedObjectsHandledCorrectlyDuringLayout) {
  SetBodyInnerHTML(R"HTML(
    <style>.container { height: 200%; }
    #sticky { position: sticky; top: 0; height: 50px; }</style>
    <div class='container'><div id='sticky'></div></div>
  )HTML");

  LayoutBoxModelObject* sticky = ToLayoutBoxModelObject(
      GetDocument().getElementById("sticky")->GetLayoutObject());

  // Deliberately invalidate the ancestor overflow layer. This approximates
  // http://crbug.com/696173, in which the ancestor overflow layer can be null
  // during layout.
  sticky->Layer()->UpdateAncestorOverflowLayer(nullptr);

  // This call should not crash.
  GetDocument().View()->LayoutViewportScrollableArea()->SetScrollOffset(
      ScrollOffset(0, 100), kProgrammaticScroll);
}

TEST_F(LocalFrameViewTest, UpdateLifecyclePhasesForPrintingDetachedFrame) {
  SetBodyInnerHTML("<iframe style='display: none'></iframe>");
  SetChildFrameHTML("A");

  ChildDocument().SetPrinting(Document::kPrinting);
  ChildDocument().View()->UpdateLifecyclePhasesForPrinting();

  // The following checks that the detached frame has been walked for PrePaint.
  EXPECT_EQ(DocumentLifecycle::kPrePaintClean,
            GetDocument().Lifecycle().GetState());
  EXPECT_EQ(DocumentLifecycle::kPrePaintClean,
            ChildDocument().Lifecycle().GetState());
  auto* child_layout_view = ChildDocument().GetLayoutView();
  EXPECT_TRUE(child_layout_view->FirstFragment().PaintProperties());
}

}  // namespace
}  // namespace blink

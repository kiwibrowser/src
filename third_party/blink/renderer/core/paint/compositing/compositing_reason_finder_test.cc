// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/compositing/compositing_reason_finder.h"

#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/layout/layout_block.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/scroll/scroll_types.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"

namespace blink {

class CompositingReasonFinderTest : public RenderingTest {
 public:
  CompositingReasonFinderTest()
      : RenderingTest(SingleChildLocalFrameClient::Create()) {}

 private:
  void SetUp() override {
    RenderingTest::SetUp();
    EnableCompositing();
  }
};

class CompositingReasonFinderTestPlatform : public TestingPlatformSupport {
 public:
  bool IsLowEndDevice() override { return true; }
};

TEST_F(CompositingReasonFinderTest, DontPromoteTrivial3DLowEnd) {
  ScopedTestingPlatformSupport<CompositingReasonFinderTestPlatform> platform;

  SetBodyInnerHTML(R"HTML(
    <div id='target'
      style='width: 100px; height: 100px; transform: translateZ(0)'></div>
  )HTML");

  Element* target = GetDocument().getElementById("target");
  PaintLayer* paint_layer =
      ToLayoutBoxModelObject(target->GetLayoutObject())->Layer();
  EXPECT_EQ(kNotComposited, paint_layer->GetCompositingState());
}

TEST_F(CompositingReasonFinderTest, PromoteNonTrivial3DLowEnd) {
  ScopedTestingPlatformSupport<CompositingReasonFinderTestPlatform> platform;

  SetBodyInnerHTML(R"HTML(
    <div id='target'
      style='width: 100px; height: 100px; transform: translateZ(1px)'></div>
  )HTML");

  Element* target = GetDocument().getElementById("target");
  PaintLayer* paint_layer =
      ToLayoutBoxModelObject(target->GetLayoutObject())->Layer();
  EXPECT_EQ(kPaintsIntoOwnBacking, paint_layer->GetCompositingState());
}

TEST_F(CompositingReasonFinderTest, PromoteTrivial3DByDefault) {
  SetBodyInnerHTML(R"HTML(
    <div id='target'
      style='width: 100px; height: 100px; transform: translateZ(0)'></div>
  )HTML");

  Element* target = GetDocument().getElementById("target");
  PaintLayer* paint_layer =
      ToLayoutBoxModelObject(target->GetLayoutObject())->Layer();
  EXPECT_EQ(kPaintsIntoOwnBacking, paint_layer->GetCompositingState());
}

TEST_F(CompositingReasonFinderTest, PromoteNonTrivial3DByDefault) {
  SetBodyInnerHTML(R"HTML(
    <div id='target'
      style='width: 100px; height: 100px; transform: translateZ(1px)'></div>
  )HTML");

  Element* target = GetDocument().getElementById("target");
  PaintLayer* paint_layer =
      ToLayoutBoxModelObject(target->GetLayoutObject())->Layer();
  EXPECT_EQ(kPaintsIntoOwnBacking, paint_layer->GetCompositingState());
}

TEST_F(CompositingReasonFinderTest, PromoteOpaqueFixedPosition) {
  ScopedCompositeOpaqueFixedPositionForTest composite_fixed_position(true);

  SetBodyInnerHTML(R"HTML(
    <div id='translucent' style='width: 20px; height: 20px; position:
    fixed; top: 100px; left: 100px;'></div>
    <div id='opaque' style='width: 20px; height: 20px; position: fixed;
    top: 100px; left: 200px; background: white;'></div>
    <div id='opaque-with-shadow' style='width: 20px; height: 20px;
    position: fixed; top: 100px; left: 300px; background: white;
    box-shadow: 10px 10px 5px #888888;'></div>
    <div id='spacer' style='height: 2000px'></div>
  )HTML");

  // The translucent fixed box should not be promoted.
  Element* element = GetDocument().getElementById("translucent");
  PaintLayer* paint_layer =
      ToLayoutBoxModelObject(element->GetLayoutObject())->Layer();
  EXPECT_EQ(kNotComposited, paint_layer->GetCompositingState());

  // The opaque fixed box should be promoted and be opaque so that text will be
  // drawn with subpixel anti-aliasing.
  element = GetDocument().getElementById("opaque");
  paint_layer = ToLayoutBoxModelObject(element->GetLayoutObject())->Layer();
  EXPECT_EQ(kPaintsIntoOwnBacking, paint_layer->GetCompositingState());
  EXPECT_TRUE(paint_layer->GraphicsLayerBacking()->ContentsOpaque());

  // The opaque fixed box with shadow should not be promoted because the layer
  // will include the shadow which is not opaque.
  element = GetDocument().getElementById("opaque-with-shadow");
  paint_layer = ToLayoutBoxModelObject(element->GetLayoutObject())->Layer();
  EXPECT_EQ(kNotComposited, paint_layer->GetCompositingState());
}

TEST_F(CompositingReasonFinderTest, OnlyAnchoredStickyPositionPromoted) {
  SetBodyInnerHTML(R"HTML(
    <style>
    .scroller {contain: paint; width: 400px; height: 400px; overflow: auto;
    will-change: transform;}
    .sticky { position: sticky; width: 10px; height: 10px;}</style>
    <div class='scroller'>
      <div id='sticky-top' class='sticky' style='top: 0px;'></div>
      <div id='sticky-no-anchor' class='sticky'></div>
      <div style='height: 2000px;'></div>
    </div>
  )HTML");

  EXPECT_EQ(kPaintsIntoOwnBacking,
            ToLayoutBoxModelObject(GetLayoutObjectByElementId("sticky-top"))
                ->Layer()
                ->GetCompositingState());
  EXPECT_EQ(kNotComposited, ToLayoutBoxModelObject(
                                GetLayoutObjectByElementId("sticky-no-anchor"))
                                ->Layer()
                                ->GetCompositingState());
}

TEST_F(CompositingReasonFinderTest, OnlyScrollingStickyPositionPromoted) {
  SetBodyInnerHTML(R"HTML(
    <style>.scroller {width: 400px; height: 400px; overflow: auto;
    will-change: transform;}
    .sticky { position: sticky; top: 0; width: 10px; height: 10px;}
    </style>
    <div class='scroller'>
      <div id='sticky-scrolling' class='sticky'></div>
      <div style='height: 2000px;'></div>
    </div>
    <div class='scroller'>
      <div id='sticky-no-scrolling' class='sticky'></div>
    </div>
  )HTML");

  EXPECT_EQ(
      kPaintsIntoOwnBacking,
      ToLayoutBoxModelObject(GetLayoutObjectByElementId("sticky-scrolling"))
          ->Layer()
          ->GetCompositingState());
  EXPECT_EQ(
      kNotComposited,
      ToLayoutBoxModelObject(GetLayoutObjectByElementId("sticky-no-scrolling"))
          ->Layer()
          ->GetCompositingState());
}

// Tests that a transform on the fixed or an ancestor will prevent promotion
// TODO(flackr): Allow integer transforms as long as all of the ancestor
// transforms are also integer.
TEST_F(CompositingReasonFinderTest, OnlyNonTransformedFixedLayersPromoted) {
  ScopedCompositeOpaqueFixedPositionForTest composite_fixed_position(true);

  SetBodyInnerHTML(R"HTML(
    <style>
    #fixed { position: fixed; height: 200px; width: 200px; background:
    white; top: 0; }
    #spacer { height: 3000px; }
    </style>
    <div id="parent">
      <div id="fixed"></div>
      <div id="spacer"></div>
    </div>
  )HTML");

  EXPECT_TRUE(RuntimeEnabledFeatures::CompositeOpaqueScrollersEnabled());
  Element* parent = GetDocument().getElementById("parent");
  Element* fixed = GetDocument().getElementById("fixed");
  PaintLayer* paint_layer =
      ToLayoutBoxModelObject(fixed->GetLayoutObject())->Layer();
  ASSERT_TRUE(paint_layer);
  EXPECT_EQ(kPaintsIntoOwnBacking, paint_layer->GetCompositingState());
  EXPECT_TRUE(paint_layer->GraphicsLayerBacking()->ContentsOpaque());

  // Change the parent to have a transform.
  parent->setAttribute(HTMLNames::styleAttr, "transform: translate(1px, 0);");
  GetDocument().View()->UpdateAllLifecyclePhases();
  paint_layer = ToLayoutBoxModelObject(fixed->GetLayoutObject())->Layer();
  ASSERT_TRUE(paint_layer);
  EXPECT_EQ(kNotComposited, paint_layer->GetCompositingState());

  // Change the parent to have no transform again.
  parent->removeAttribute(HTMLNames::styleAttr);
  GetDocument().View()->UpdateAllLifecyclePhases();
  paint_layer = ToLayoutBoxModelObject(fixed->GetLayoutObject())->Layer();
  ASSERT_TRUE(paint_layer);
  EXPECT_EQ(kPaintsIntoOwnBacking, paint_layer->GetCompositingState());
  EXPECT_TRUE(paint_layer->GraphicsLayerBacking()->ContentsOpaque());

  // Apply a transform to the fixed directly.
  fixed->setAttribute(HTMLNames::styleAttr, "transform: translate(1px, 0);");
  GetDocument().View()->UpdateAllLifecyclePhases();
  paint_layer = ToLayoutBoxModelObject(fixed->GetLayoutObject())->Layer();
  ASSERT_TRUE(paint_layer);
  EXPECT_EQ(kNotComposited, paint_layer->GetCompositingState());
}

// Test that opacity applied to the fixed or an ancestor will cause the
// scrolling contents layer to not be promoted.
TEST_F(CompositingReasonFinderTest, OnlyOpaqueFixedLayersPromoted) {
  ScopedCompositeOpaqueFixedPositionForTest composite_fixed_position(true);

  SetBodyInnerHTML(R"HTML(
    <style>
    #fixed { position: fixed; height: 200px; width: 200px; background:
    white; top: 0}
    #spacer { height: 3000px; }
    </style>
    <div id="parent">
      <div id="fixed"></div>
      <div id="spacer"></div>
    </div>
  )HTML");

  EXPECT_TRUE(RuntimeEnabledFeatures::CompositeOpaqueScrollersEnabled());
  Element* parent = GetDocument().getElementById("parent");
  Element* fixed = GetDocument().getElementById("fixed");
  PaintLayer* paint_layer =
      ToLayoutBoxModelObject(fixed->GetLayoutObject())->Layer();
  ASSERT_TRUE(paint_layer);
  EXPECT_EQ(kPaintsIntoOwnBacking, paint_layer->GetCompositingState());
  EXPECT_TRUE(paint_layer->GraphicsLayerBacking()->ContentsOpaque());

  // Change the parent to be partially translucent.
  parent->setAttribute(HTMLNames::styleAttr, "opacity: 0.5;");
  GetDocument().View()->UpdateAllLifecyclePhases();
  paint_layer = ToLayoutBoxModelObject(fixed->GetLayoutObject())->Layer();
  ASSERT_TRUE(paint_layer);
  EXPECT_EQ(kNotComposited, paint_layer->GetCompositingState());

  // Change the parent to be opaque again.
  parent->setAttribute(HTMLNames::styleAttr, "opacity: 1;");
  GetDocument().View()->UpdateAllLifecyclePhases();
  paint_layer = ToLayoutBoxModelObject(fixed->GetLayoutObject())->Layer();
  ASSERT_TRUE(paint_layer);
  EXPECT_EQ(kPaintsIntoOwnBacking, paint_layer->GetCompositingState());
  EXPECT_TRUE(paint_layer->GraphicsLayerBacking()->ContentsOpaque());

  // Make the fixed translucent.
  fixed->setAttribute(HTMLNames::styleAttr, "opacity: 0.5");
  GetDocument().View()->UpdateAllLifecyclePhases();
  paint_layer = ToLayoutBoxModelObject(fixed->GetLayoutObject())->Layer();
  ASSERT_TRUE(paint_layer);
  EXPECT_EQ(kNotComposited, paint_layer->GetCompositingState());
}

TEST_F(CompositingReasonFinderTest, RequiresCompositingForTransformAnimation) {
  scoped_refptr<ComputedStyle> style = ComputedStyle::Create();
  style->SetSubtreeWillChangeContents(false);

  style->SetHasCurrentTransformAnimation(false);
  style->SetIsRunningTransformAnimationOnCompositor(false);
  EXPECT_FALSE(
      CompositingReasonFinder::RequiresCompositingForTransformAnimation(
          *style));

  style->SetHasCurrentTransformAnimation(false);
  style->SetIsRunningTransformAnimationOnCompositor(true);
  EXPECT_FALSE(
      CompositingReasonFinder::RequiresCompositingForTransformAnimation(
          *style));

  style->SetHasCurrentTransformAnimation(true);
  style->SetIsRunningTransformAnimationOnCompositor(false);
  EXPECT_TRUE(CompositingReasonFinder::RequiresCompositingForTransformAnimation(
      *style));

  style->SetHasCurrentTransformAnimation(true);
  style->SetIsRunningTransformAnimationOnCompositor(true);
  EXPECT_TRUE(CompositingReasonFinder::RequiresCompositingForTransformAnimation(
      *style));

  style->SetSubtreeWillChangeContents(true);

  style->SetHasCurrentTransformAnimation(false);
  style->SetIsRunningTransformAnimationOnCompositor(false);
  EXPECT_FALSE(
      CompositingReasonFinder::RequiresCompositingForTransformAnimation(
          *style));

  style->SetHasCurrentTransformAnimation(false);
  style->SetIsRunningTransformAnimationOnCompositor(true);
  EXPECT_TRUE(CompositingReasonFinder::RequiresCompositingForTransformAnimation(
      *style));

  style->SetHasCurrentTransformAnimation(true);
  style->SetIsRunningTransformAnimationOnCompositor(false);
  EXPECT_FALSE(
      CompositingReasonFinder::RequiresCompositingForTransformAnimation(
          *style));

  style->SetHasCurrentTransformAnimation(true);
  style->SetIsRunningTransformAnimationOnCompositor(true);
  EXPECT_TRUE(CompositingReasonFinder::RequiresCompositingForTransformAnimation(
      *style));
}

TEST_F(CompositingReasonFinderTest, CompositingReasonsForAnimation) {
  scoped_refptr<ComputedStyle> style = ComputedStyle::Create();

  style->SetSubtreeWillChangeContents(false);
  style->SetHasCurrentTransformAnimation(false);
  style->SetHasCurrentOpacityAnimation(false);
  style->SetHasCurrentFilterAnimation(false);
  style->SetHasCurrentBackdropFilterAnimation(false);
  EXPECT_EQ(CompositingReason::kNone,
            CompositingReasonFinder::CompositingReasonsForAnimation(*style));

  style->SetHasCurrentTransformAnimation(true);
  EXPECT_EQ(CompositingReason::kActiveTransformAnimation,
            CompositingReasonFinder::CompositingReasonsForAnimation(*style));

  style->SetHasCurrentOpacityAnimation(true);
  EXPECT_EQ(CompositingReason::kActiveTransformAnimation |
                CompositingReason::kActiveOpacityAnimation,
            CompositingReasonFinder::CompositingReasonsForAnimation(*style));

  style->SetHasCurrentFilterAnimation(true);
  EXPECT_EQ(CompositingReason::kActiveTransformAnimation |
                CompositingReason::kActiveOpacityAnimation |
                CompositingReason::kActiveFilterAnimation,
            CompositingReasonFinder::CompositingReasonsForAnimation(*style));

  style->SetHasCurrentBackdropFilterAnimation(true);
  EXPECT_EQ(CompositingReason::kActiveTransformAnimation |
                CompositingReason::kActiveOpacityAnimation |
                CompositingReason::kActiveFilterAnimation |
                CompositingReason::kActiveBackdropFilterAnimation,
            CompositingReasonFinder::CompositingReasonsForAnimation(*style));
  EXPECT_EQ(CompositingReason::kComboActiveAnimation,
            CompositingReasonFinder::CompositingReasonsForAnimation(*style));
}

TEST_F(CompositingReasonFinderTest, CompositeNestedSticky) {
  ScopedCompositeOpaqueFixedPositionForTest composite_fixed_position(true);

  SetBodyInnerHTML(R"HTML(
    <style>.scroller { overflow: scroll; height: 200px; width: 100px; }
    .container { height: 500px; }
    .opaque { background-color: white; contain: paint; }
    #outerSticky { height: 50px; position: sticky; top: 0px; }
    #innerSticky { height: 20px; position: sticky; top: 25px; }</style>
    <div class='scroller'>
      <div class='container'>
        <div id='outerSticky' class='opaque'>
          <div id='innerSticky' class='opaque'></div>
        </div>
      </div>
    </div>
  )HTML");

  Element* outer_sticky = GetDocument().getElementById("outerSticky");
  PaintLayer* outer_sticky_layer =
      ToLayoutBoxModelObject(outer_sticky->GetLayoutObject())->Layer();
  ASSERT_TRUE(outer_sticky_layer);

  Element* inner_sticky = GetDocument().getElementById("innerSticky");
  PaintLayer* inner_sticky_layer =
      ToLayoutBoxModelObject(inner_sticky->GetLayoutObject())->Layer();
  ASSERT_TRUE(inner_sticky_layer);

  EXPECT_EQ(kPaintsIntoOwnBacking, outer_sticky_layer->GetCompositingState());
  EXPECT_EQ(kPaintsIntoOwnBacking, inner_sticky_layer->GetCompositingState());
}

TEST_F(CompositingReasonFinderTest, DontPromoteEmptyIframe) {
  GetDocument().GetFrame()->GetSettings()->SetPreferCompositingToLCDTextEnabled(
      true);

  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <iframe style="width:0; height:0; border: 0;" srcdoc="<!DOCTYPE html>"></iframe>
  )HTML");
  GetDocument().View()->UpdateAllLifecyclePhases();

  LocalFrame* child_frame =
      ToLocalFrame(GetDocument().GetFrame()->Tree().FirstChild());
  ASSERT_TRUE(child_frame);
  LocalFrameView* child_frame_view = child_frame->View();
  ASSERT_TRUE(child_frame_view);
  EXPECT_EQ(kNotComposited,
            child_frame_view->GetLayoutView()->Layer()->GetCompositingState());
}

}  // namespace blink

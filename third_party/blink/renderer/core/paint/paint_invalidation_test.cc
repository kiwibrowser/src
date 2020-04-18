// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/platform/graphics/compositing/paint_artifact_compositor.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/raster_invalidation_tracking.h"
#include "third_party/blink/renderer/platform/testing/paint_test_configurations.h"

namespace blink {

namespace {

class PaintInvalidationTest : public PaintTestConfigurations,
                              public RenderingTest {
 public:
  PaintInvalidationTest()
      : RenderingTest(SingleChildLocalFrameClient::Create()) {}

 protected:
  const RasterInvalidationTracking* GetRasterInvalidationTracking() const {
    // TODO(wangxianzhu): Test raster invalidation for SPv2 (in this test suite
    // or elsewhere for the applicable cases in this test suite).
    DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV2Enabled());
    return GetLayoutView()
        .Layer()
        ->GraphicsLayerBacking()
        ->GetRasterInvalidationTracking();
  }
};

INSTANTIATE_PAINT_TEST_CASE_P(PaintInvalidationTest);

// Changing style in a way that changes overflow without layout should cause
// the layout view to possibly need a paint invalidation since we may have
// revealed additional background that can be scrolled into view.
TEST_P(PaintInvalidationTest, RecalcOverflowInvalidatesBackground) {
  GetDocument().GetPage()->GetSettings().SetViewportEnabled(true);
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style type='text/css'>
      body, html {
        width: 100%;
        height: 100%;
        margin: 0px;
      }
      #container {
        will-change: transform;
        width: 100%;
        height: 100%;
      }
    </style>
    <div id='container'></div>
  )HTML");

  GetDocument().View()->UpdateAllLifecyclePhases();

  ScrollableArea* scrollable_area =
      GetDocument().View()->LayoutViewportScrollableArea();
  ASSERT_EQ(scrollable_area->MaximumScrollOffset().Height(), 0);
  EXPECT_FALSE(GetDocument().GetLayoutView()->MayNeedPaintInvalidation());

  Element* container = GetDocument().getElementById("container");
  container->setAttribute(HTMLNames::styleAttr,
                          "transform: translateY(1000px);");
  GetDocument().UpdateStyleAndLayoutTree();

  EXPECT_EQ(scrollable_area->MaximumScrollOffset().Height(), 1000);
  EXPECT_TRUE(GetDocument().GetLayoutView()->MayNeedPaintInvalidation());
}

TEST_P(PaintInvalidationTest, UpdateVisualRectOnFrameBorderWidthChange) {
  // TODO(wangxianzhu): enable for SPv2.
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  SetBodyInnerHTML(R"HTML(
    <style>
      body { margin: 10px }
      iframe { width: 100px; height: 100px; border: none; }
    </style>
    <iframe id='iframe'></iframe>
  )HTML");

  Element* iframe = GetDocument().getElementById("iframe");
  LayoutView* child_layout_view = ChildDocument().GetLayoutView();
  EXPECT_EQ(GetDocument().GetLayoutView(),
            &child_layout_view->ContainerForPaintInvalidation());
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    EXPECT_EQ(LayoutRect(0, 0, 100, 100),
              child_layout_view->FirstFragment().VisualRect());
  } else {
    EXPECT_EQ(LayoutRect(10, 10, 100, 100),
              child_layout_view->FirstFragment().VisualRect());
  }

  iframe->setAttribute(HTMLNames::styleAttr, "border: 20px solid blue");
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_EQ(GetDocument().GetLayoutView(),
            &child_layout_view->ContainerForPaintInvalidation());
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    EXPECT_EQ(LayoutRect(0, 0, 100, 100),
              child_layout_view->FirstFragment().VisualRect());
  } else {
    EXPECT_EQ(LayoutRect(30, 30, 100, 100),
              child_layout_view->FirstFragment().VisualRect());
  }
};

// This is a simplified test case for crbug.com/704182. It ensures no repaint
// on transform change causing no visual change.
TEST_P(PaintInvalidationTest, InvisibleTransformUnderFixedOnScroll) {
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  EnableCompositing();
  SetBodyInnerHTML(R"HTML(
    <style>
      #fixed {
        position: fixed;
        top: 0;
        left: 0;
        width: 100px;
        height: 100px;
        background-color: blue;
      }
      #transform {
        width: 100px;
        height: 100px;
        background-color: yellow;
        will-change: transform;
        transform: translate(10px, 20px);
      }
    </style>
    <div style='height: 2000px'></div>
    <div id='fixed' style='visibility: hidden'>
      <div id='transform'></div>
    </div>
  )HTML");

  auto& fixed = *GetDocument().getElementById("fixed");
  const auto& fixed_object = ToLayoutBox(*fixed.GetLayoutObject());
  const auto& fixed_layer = *fixed_object.Layer();
  auto& transform = *GetDocument().getElementById("transform");
  EXPECT_TRUE(fixed_layer.SubtreeIsInvisible());
  EXPECT_EQ(LayoutRect(0, 0, 110, 120), fixed_object.LayoutOverflowRect());

  GetDocument().domWindow()->scrollTo(0, 100);
  transform.setAttribute(HTMLNames::styleAttr,
                         "transform: translate(20px, 30px)");
  GetDocument().View()->UpdateLifecycleToCompositingCleanPlusScrolling();

  EXPECT_TRUE(fixed_layer.SubtreeIsInvisible());
  // We skip invisible layers when setting non-composited fixed-position
  // needing paint invalidation when the frame is scrolled.
  EXPECT_FALSE(fixed_object.ShouldDoFullPaintInvalidation());
  // This was set when fixedObject is marked needsOverflowRecaldAfterStyleChange
  // when child changed transform.
  EXPECT_TRUE(fixed_object.MayNeedPaintInvalidation());
  EXPECT_EQ(LayoutRect(0, 0, 120, 130), fixed_object.LayoutOverflowRect());

  GetDocument().View()->UpdateAllLifecyclePhasesExceptPaint();
  // Invalidation is still needed for invisible transformed content, because it
  // may end up composited (in SPv2 mode) and move on screen.
  EXPECT_TRUE(fixed_layer.NeedsRepaint());
  GetDocument().View()->UpdateAllLifecyclePhases();

  // The following ensures normal paint invalidation still works.
  transform.setAttribute(
      HTMLNames::styleAttr,
      "visibility: visible; transform: translate(20px, 30px)");
  GetDocument().View()->UpdateLifecycleToCompositingCleanPlusScrolling();
  EXPECT_FALSE(fixed_layer.SubtreeIsInvisible());
  GetDocument().View()->UpdateAllLifecyclePhases();
  fixed.setAttribute(HTMLNames::styleAttr, "top: 50px");
  GetDocument().View()->UpdateLifecycleToCompositingCleanPlusScrolling();
  EXPECT_TRUE(fixed_object.MayNeedPaintInvalidation());
  EXPECT_FALSE(fixed_layer.SubtreeIsInvisible());
  GetDocument().View()->UpdateAllLifecyclePhasesExceptPaint();
  EXPECT_TRUE(fixed_layer.NeedsRepaint());
  GetDocument().View()->UpdateAllLifecyclePhases();
}

TEST_P(PaintInvalidationTest, DelayedFullPaintInvalidation) {
  EnableCompositing();
  SetBodyInnerHTML(R"HTML(
    <style>body { margin: 0 }</style>
    <div style='height: 4000px'></div>
    <div id='target' style='width: 100px; height: 100px; background: blue'>
    </div>
  )HTML");

  auto* target = GetLayoutObjectByElementId("target");
  target->SetShouldDoFullPaintInvalidationWithoutGeometryChange(
      PaintInvalidationReason::kDelayedFull);
  EXPECT_EQ(PaintInvalidationReason::kDelayedFull,
            target->FullPaintInvalidationReason());
  EXPECT_FALSE(target->NeedsPaintOffsetAndVisualRectUpdate());

  GetDocument().View()->SetTracksPaintInvalidations(true);
  GetDocument().View()->UpdateAllLifecyclePhases();
  if (!RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    EXPECT_FALSE(GetRasterInvalidationTracking()->HasInvalidations());
  EXPECT_EQ(PaintInvalidationReason::kDelayedFull,
            target->FullPaintInvalidationReason());
  EXPECT_FALSE(target->NeedsPaintOffsetAndVisualRectUpdate());
  GetDocument().View()->SetTracksPaintInvalidations(false);

  GetDocument().View()->SetTracksPaintInvalidations(true);
  // Scroll target into view.
  GetDocument().domWindow()->scrollTo(0, 4000);
  GetDocument().View()->UpdateAllLifecyclePhases();
  if (!RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
    const auto& raster_invalidations =
        GetRasterInvalidationTracking()->Invalidations();
    ASSERT_EQ(1u, raster_invalidations.size());
    EXPECT_EQ(PaintInvalidationReason::kNone,
              target->FullPaintInvalidationReason());
    EXPECT_EQ(IntRect(0, 4000, 100, 100), raster_invalidations[0].rect);
    EXPECT_EQ(PaintInvalidationReason::kFull, raster_invalidations[0].reason);
  }
  EXPECT_FALSE(target->NeedsPaintOffsetAndVisualRectUpdate());
  GetDocument().View()->SetTracksPaintInvalidations(false);
};

TEST_P(PaintInvalidationTest, SVGHiddenContainer) {
  EnableCompositing();
  SetBodyInnerHTML(R"HTML(
    <svg style='position: absolute; top: 100px; left: 100px'>
      <mask id='mask'>
        <g transform='scale(2)'>
          <rect id='mask-rect' x='11' y='22' width='33' height='44'/>
        </g>
      </mask>
      <rect id='real-rect' x='55' y='66' width='7' height='8'
          mask='url(#mask)'/>
    </svg>
  )HTML");

  // mask_rect's visual rect is in coordinates of the mask.
  auto* mask_rect = GetLayoutObjectByElementId("mask-rect");
  EXPECT_EQ(LayoutRect(), mask_rect->FirstFragment().VisualRect());

  // real_rect's visual rect is in coordinates of its paint invalidation
  // container (the view).
  auto* real_rect = GetLayoutObjectByElementId("real-rect");
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    EXPECT_EQ(LayoutRect(55, 66, 7, 8),
              real_rect->FirstFragment().VisualRect());
  } else {
    EXPECT_EQ(LayoutRect(155, 166, 7, 8),
              real_rect->FirstFragment().VisualRect());
  }

  GetDocument().View()->SetTracksPaintInvalidations(true);
  ToElement(mask_rect->GetNode())->setAttribute("x", "20");
  GetDocument().View()->UpdateAllLifecyclePhasesExceptPaint();

  EXPECT_EQ(PaintInvalidationReason::kFull,
            real_rect->GetPaintInvalidationReason());
  // mask_rect has PaintInvalidationReason::kFull because it is not cached by
  // any PaintController.
  EXPECT_EQ(PaintInvalidationReason::kFull,
            mask_rect->GetPaintInvalidationReason());

  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    GetDocument().View()->UpdateAllLifecyclePhases();

  EXPECT_EQ(LayoutRect(), mask_rect->FirstFragment().VisualRect());
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    EXPECT_EQ(LayoutRect(55, 66, 7, 8),
              real_rect->FirstFragment().VisualRect());
  } else {
    EXPECT_EQ(LayoutRect(155, 166, 7, 8),
              real_rect->FirstFragment().VisualRect());
  }

  // Should invalidate raster for real_rect only.
  if (!RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
    const auto& raster_invalidations =
        GetRasterInvalidationTracking()->Invalidations();
    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
      // The first is for the rect itself, the second is for its mask.
      ASSERT_EQ(2u, raster_invalidations.size());
      EXPECT_EQ(IntRect(155, 166, 7, 8), raster_invalidations[0].rect);
      EXPECT_EQ(PaintInvalidationReason::kFull, raster_invalidations[0].reason);
      EXPECT_EQ(IntRect(155, 166, 7, 8), raster_invalidations[1].rect);
      EXPECT_EQ(PaintInvalidationReason::kFull, raster_invalidations[1].reason);
    } else {
      ASSERT_EQ(1u, raster_invalidations.size());
      EXPECT_EQ(IntRect(155, 166, 7, 8), raster_invalidations[0].rect);
      EXPECT_EQ(PaintInvalidationReason::kFull, raster_invalidations[0].reason);
    }
  }

  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintInvalidationTest, UpdateVisualRectWhenPrinting) {
  SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0;}
      span {
        display: inline-block;
        width: 150px;
        height: 20px;
        background: rebeccapurple;
      }
    </style>
    <div><span id="a"></span><span id="b"></span><span id="c"></div>
  )HTML");

  auto* a = GetDocument().getElementById("a")->GetLayoutObject();
  EXPECT_EQ(LayoutRect(0, 0, 150, 20), a->FirstFragment().VisualRect());
  auto* b = GetDocument().getElementById("b")->GetLayoutObject();
  EXPECT_EQ(LayoutRect(150, 0, 150, 20), b->FirstFragment().VisualRect());
  auto* c = GetDocument().getElementById("c")->GetLayoutObject();
  EXPECT_EQ(LayoutRect(300, 0, 150, 20), c->FirstFragment().VisualRect());

  // Print the page with a width of 400px which will require wrapping 'c'.
  FloatSize page_size(400, 200);
  GetFrame().StartPrinting(page_size, page_size, 1);
  GetDocument().View()->UpdateLifecyclePhasesForPrinting();

  EXPECT_EQ(LayoutRect(0, 0, 150, 20), a->FirstFragment().VisualRect());
  EXPECT_EQ(LayoutRect(150, 0, 150, 20), b->FirstFragment().VisualRect());
  // 'c' should be on the next line.
  EXPECT_EQ(LayoutRect(0, 20, 150, 20), c->FirstFragment().VisualRect());

  GetFrame().EndPrinting();
  GetDocument().View()->UpdateLifecyclePhasesForPrinting();

  EXPECT_EQ(LayoutRect(0, 0, 150, 20), a->FirstFragment().VisualRect());
  EXPECT_EQ(LayoutRect(150, 0, 150, 20), b->FirstFragment().VisualRect());
  EXPECT_EQ(LayoutRect(300, 0, 150, 20), c->FirstFragment().VisualRect());
};

}  // namespace

}  // namespace blink

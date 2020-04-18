// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/box_paint_invalidator.h"

#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/paint_controller_paint_test.h"
#include "third_party/blink/renderer/core/paint/paint_invalidator.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/raster_invalidation_tracking.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

namespace blink {

class BoxPaintInvalidatorTest : public PaintControllerPaintTest {
 public:
  BoxPaintInvalidatorTest()
      : PaintControllerPaintTest(SingleChildLocalFrameClient::Create()) {}

 protected:
  const RasterInvalidationTracking* GetRasterInvalidationTracking() const {
    // TODO(wangxianzhu): Test SPv2.
    // TODO(wangxianzhu): For SPv175, XXXPaintInvalidator doesn't do raster
    // invalidation, so the test should be moved to somewhere else (or use
    // layout tests?).
    return GetLayoutView()
        .Layer()
        ->GraphicsLayerBacking()
        ->GetRasterInvalidationTracking();
  }

  PaintInvalidationReason ComputePaintInvalidationReason(
      const LayoutBox& box,
      const LayoutRect& old_visual_rect,
      const LayoutPoint& old_location) {
    FragmentData fragment_data;
    PaintInvalidatorContext context;
    context.old_visual_rect = old_visual_rect;
    context.old_location = old_location;
    fragment_data_.SetVisualRect(box.FirstFragment().VisualRect());
    fragment_data_.SetLocationInBacking(
        box.FirstFragment().LocationInBacking());
    context.fragment_data = &fragment_data_;
    return BoxPaintInvalidator(box, context).ComputePaintInvalidationReason();
  }

  // This applies when the target is set to meet conditions that we should do
  // full paint invalidation instead of incremental invalidation on geometry
  // change.
  void ExpectFullPaintInvalidationOnGeometryChange(const char* test_title) {
    SCOPED_TRACE(test_title);

    GetDocument().View()->UpdateAllLifecyclePhases();
    auto& target = *GetDocument().getElementById("target");
    const auto& box = *ToLayoutBox(target.GetLayoutObject());
    LayoutRect visual_rect = box.FirstFragment().VisualRect();
    LayoutPoint location = box.FirstFragment().LocationInBacking();

    // No geometry change.
    EXPECT_EQ(PaintInvalidationReason::kNone,
              ComputePaintInvalidationReason(box, visual_rect, location));

    target.setAttribute(
        HTMLNames::styleAttr,
        target.getAttribute(HTMLNames::styleAttr) + "; width: 200px");
    GetDocument().View()->UpdateLifecycleToLayoutClean();
    // Simulate that PaintInvalidator updates visual rect.
    box.GetMutableForPainting().SetVisualRect(
        LayoutRect(visual_rect.Location(), box.Size()));

    EXPECT_EQ(PaintInvalidationReason::kGeometry,
              ComputePaintInvalidationReason(box, visual_rect, location));
  }

 private:
  void SetUp() override {
    RenderingTest::SetUp();
    EnableCompositing();
    SetBodyInnerHTML(R"HTML(
      <style>
        body {
          margin: 0;
          height: 0;
        }
        ::-webkit-scrollbar { display: none }
        #target {
          width: 50px;
          height: 100px;
          transform-origin: 0 0;
        }
        .border {
          border-width: 20px 10px;
          border-style: solid;
          border-color: red;
        }
        .solid-composited-scroller {
          overflow: scroll;
          will-change: transform;
          background: #ccc;
        }
        .local-background {
          background-attachment: local;
          overflow: scroll;
        }
        .gradient {
          background-image: linear-gradient(blue, yellow)
        }
        .transform {
          transform: scale(2);
        }
      </style>
      <div id='target' class='border'></div>
    )HTML");
  }
  FragmentData fragment_data_;
};

INSTANTIATE_PAINT_TEST_CASE_P(BoxPaintInvalidatorTest);

TEST_P(BoxPaintInvalidatorTest, SlowMapToVisualRectInAncestorSpaceLayoutView) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  SetBodyInnerHTML(
      "<!doctype html>"
      "<style>"
      "#parent {"
      "    display: inline-block;"
      "    width: 300px;"
      "    height: 300px;"
      "    margin-top: 200px;"
      "    filter: blur(3px);"  // Forces the slow path in
                                // SlowMapToVisualRectInAncestorSpace.
      "    border: 1px solid rebeccapurple;"
      "}"
      "</style>"
      "<div id=parent>"
      "  <iframe id='target' src='data:text/html,<body style='background: "
      "blue;'></body>'></iframe>"
      "</div>");

  auto& target = *GetDocument().getElementById("target");
  EXPECT_EQ(IntRect(2, 202, 318, 168),
            EnclosingIntRect(ToHTMLFrameOwnerElement(target)
                                 .contentDocument()
                                 ->GetLayoutView()
                                 ->FirstFragment()
                                 .VisualRect()));
}

TEST_P(BoxPaintInvalidatorTest, ComputePaintInvalidationReasonPaintingNothing) {
  auto& target = *GetDocument().getElementById("target");
  auto& box = *ToLayoutBox(target.GetLayoutObject());
  // Remove border.
  target.setAttribute(HTMLNames::classAttr, "");
  GetDocument().View()->UpdateAllLifecyclePhases();

  EXPECT_TRUE(box.PaintedOutputOfObjectHasNoEffectRegardlessOfSize());
  LayoutRect visual_rect = box.FirstFragment().VisualRect();
  EXPECT_EQ(LayoutRect(0, 0, 50, 100), visual_rect);

  // No geometry change.
  EXPECT_EQ(
      PaintInvalidationReason::kNone,
      ComputePaintInvalidationReason(box, visual_rect, visual_rect.Location()));

  // Location change.
  EXPECT_EQ(PaintInvalidationReason::kNone,
            ComputePaintInvalidationReason(
                box, visual_rect, visual_rect.Location() + LayoutSize(10, 20)));

  // Visual rect size change.
  LayoutRect old_visual_rect = visual_rect;
  target.setAttribute(HTMLNames::styleAttr, "width: 200px");
  GetDocument().View()->UpdateLifecycleToLayoutClean();
  // Simulate that PaintInvalidator updates visual rect.
  box.GetMutableForPainting().SetVisualRect(
      LayoutRect(visual_rect.Location(), box.Size()));

  EXPECT_EQ(PaintInvalidationReason::kNone,
            ComputePaintInvalidationReason(box, old_visual_rect,
                                           old_visual_rect.Location()));
}

TEST_P(BoxPaintInvalidatorTest, ComputePaintInvalidationReasonBasic) {
  auto& target = *GetDocument().getElementById("target");
  auto& box = *ToLayoutBox(target.GetLayoutObject());
  // Remove border.
  target.setAttribute(HTMLNames::classAttr, "");
  target.setAttribute(HTMLNames::styleAttr, "background: blue");
  GetDocument().View()->UpdateAllLifecyclePhases();

  box.SetMayNeedPaintInvalidation();
  LayoutRect visual_rect = box.FirstFragment().VisualRect();
  EXPECT_EQ(LayoutRect(0, 0, 50, 100), visual_rect);

  // No geometry change.
  EXPECT_EQ(
      PaintInvalidationReason::kNone,
      ComputePaintInvalidationReason(box, visual_rect, visual_rect.Location()));

  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    // Location change.
    EXPECT_EQ(
        PaintInvalidationReason::kGeometry,
        ComputePaintInvalidationReason(
            box, visual_rect, visual_rect.Location() + LayoutSize(10, 20)));
  }

  // Visual rect size change.
  LayoutRect old_visual_rect = visual_rect;
  target.setAttribute(HTMLNames::styleAttr, "background: blue; width: 200px");
  GetDocument().View()->UpdateLifecycleToLayoutClean();
  // Simulate that PaintInvalidator updates visual rect.
  box.GetMutableForPainting().SetVisualRect(
      LayoutRect(visual_rect.Location(), box.Size()));

  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            ComputePaintInvalidationReason(box, old_visual_rect,
                                           old_visual_rect.Location()));

  // Visual rect size change, with location in backing different from location
  // of visual rect.
  LayoutPoint fake_location = visual_rect.Location() + LayoutSize(10, 20);
  box.GetMutableForPainting().FirstFragment().SetLocationInBacking(
      fake_location);
  EXPECT_EQ(
      PaintInvalidationReason::kGeometry,
      ComputePaintInvalidationReason(box, old_visual_rect, fake_location));

  // Should use the existing full paint invalidation reason regardless of
  // geometry change.
  box.SetShouldDoFullPaintInvalidation(PaintInvalidationReason::kStyle);
  EXPECT_EQ(
      PaintInvalidationReason::kStyle,
      ComputePaintInvalidationReason(box, visual_rect, visual_rect.Location()));
  EXPECT_EQ(PaintInvalidationReason::kStyle,
            ComputePaintInvalidationReason(
                box, visual_rect, visual_rect.Location() + LayoutSize(10, 20)));
}

TEST_P(BoxPaintInvalidatorTest, ComputePaintInvalidationReasonOtherCases) {
  auto& target = *GetDocument().getElementById("target");

  // The target initially has border.
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    ExpectFullPaintInvalidationOnGeometryChange("With border");

  // Clear border.
  target.setAttribute(HTMLNames::classAttr, "");
  target.setAttribute(HTMLNames::styleAttr, "border-radius: 5px");
  ExpectFullPaintInvalidationOnGeometryChange("With border-radius");

  target.setAttribute(HTMLNames::styleAttr, "-webkit-mask: url(#)");
  ExpectFullPaintInvalidationOnGeometryChange("With mask");

  target.setAttribute(HTMLNames::styleAttr, "filter: blur(5px)");
  ExpectFullPaintInvalidationOnGeometryChange("With filter");

  target.setAttribute(HTMLNames::styleAttr, "outline: 2px solid blue");
  ExpectFullPaintInvalidationOnGeometryChange("With outline");

  target.setAttribute(HTMLNames::styleAttr, "box-shadow: inset 3px 2px");
  ExpectFullPaintInvalidationOnGeometryChange("With box-shadow");

  target.setAttribute(HTMLNames::styleAttr, "-webkit-appearance: button");
  ExpectFullPaintInvalidationOnGeometryChange("With appearance");

  target.setAttribute(HTMLNames::styleAttr, "clip-path: circle(50% at 0 50%)");
  ExpectFullPaintInvalidationOnGeometryChange("With clip-path");
}

TEST_P(BoxPaintInvalidatorTest, IncrementalInvalidationExpand) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  GetDocument().View()->SetTracksPaintInvalidations(true);
  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::styleAttr, "width: 100px; height: 200px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto& raster_invalidations =
      GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations.size());
  EXPECT_EQ(IntRect(60, 0, 60, 240), raster_invalidations[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[0].reason);
  EXPECT_EQ(IntRect(0, 120, 120, 120), raster_invalidations[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, IncrementalInvalidationShrink) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  GetDocument().View()->SetTracksPaintInvalidations(true);
  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::styleAttr, "width: 20px; height: 80px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto& raster_invalidations =
      GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations.size());
  EXPECT_EQ(IntRect(30, 0, 40, 140), raster_invalidations[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[0].reason);
  EXPECT_EQ(IntRect(0, 100, 70, 40), raster_invalidations[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, IncrementalInvalidationMixed) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  GetDocument().View()->SetTracksPaintInvalidations(true);
  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::styleAttr, "width: 100px; height: 80px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto& raster_invalidations =
      GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations.size());
  EXPECT_EQ(IntRect(60, 0, 60, 120), raster_invalidations[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[0].reason);
  EXPECT_EQ(IntRect(0, 100, 70, 40), raster_invalidations[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, SubpixelVisualRectChagne) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  Element* target = GetDocument().getElementById("target");

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "width: 100.6px; height: 70.3px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto* raster_invalidations =
      &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations->size());
  EXPECT_EQ(IntRect(60, 0, 61, 111), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            (*raster_invalidations)[0].reason);
  EXPECT_EQ(IntRect(0, 90, 70, 50), (*raster_invalidations)[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            (*raster_invalidations)[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "width: 50px; height: 100px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  raster_invalidations = &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations->size());
  EXPECT_EQ(IntRect(60, 0, 61, 111), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            (*raster_invalidations)[0].reason);
  EXPECT_EQ(IntRect(0, 90, 70, 50), (*raster_invalidations)[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            (*raster_invalidations)[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, SubpixelVisualRectChangeWithTransform) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::classAttr, "border transform");
  GetDocument().View()->UpdateAllLifecyclePhases();

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "width: 100.6px; height: 70.3px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto* raster_invalidations =
      &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations->size());
  EXPECT_EQ(IntRect(0, 0, 140, 280), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[0].reason);
  EXPECT_EQ(IntRect(0, 0, 242, 222), (*raster_invalidations)[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "width: 50px; height: 100px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  raster_invalidations = &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations->size());
  EXPECT_EQ(IntRect(0, 0, 242, 222), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[0].reason);
  EXPECT_EQ(IntRect(0, 0, 140, 280), (*raster_invalidations)[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, SubpixelWithinPixelsChange) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  Element* target = GetDocument().getElementById("target");
  LayoutObject* target_object = target->GetLayoutObject();
  EXPECT_EQ(LayoutRect(0, 0, 70, 140),
            target_object->FirstFragment().VisualRect());

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr,
                       "margin-top: 0.6px; width: 50px; height: 99.3px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_EQ(LayoutRect(LayoutUnit(), LayoutUnit(0.6), LayoutUnit(70),
                       LayoutUnit(139.3)),
            target_object->FirstFragment().VisualRect());
  const auto* raster_invalidations =
      &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, raster_invalidations->size());
  EXPECT_EQ(IntRect(0, 0, 70, 140), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr,
                       "margin-top: 0.6px; width: 49.3px; height: 98.5px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_EQ(LayoutRect(LayoutUnit(), LayoutUnit(0.6), LayoutUnit(69.3),
                       LayoutUnit(138.5)),
            target_object->FirstFragment().VisualRect());
  raster_invalidations = &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations->size());
  EXPECT_EQ(IntRect(59, 0, 11, 140), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            (*raster_invalidations)[0].reason);
  EXPECT_EQ(IntRect(0, 119, 70, 21), (*raster_invalidations)[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            (*raster_invalidations)[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, ResizeRotated) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::styleAttr, "transform: rotate(45deg)");
  GetDocument().View()->UpdateAllLifecyclePhases();

  // Should do full invalidation a rotated object is resized.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr,
                       "transform: rotate(45deg); width: 200px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto* raster_invalidations =
      &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, raster_invalidations->size());
  EXPECT_EQ(IntRect(-99, 0, 255, 255), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, ResizeRotatedChild) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::styleAttr,
                       "transform: rotate(45deg); width: 200px");
  target->SetInnerHTMLFromString(
      "<div id=child style='width: 50px; height: 50px; background: "
      "red'></div>");
  GetDocument().View()->UpdateAllLifecyclePhases();
  Element* child = GetDocument().getElementById("child");

  // Should do full invalidation a rotated object is resized.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  child->setAttribute(HTMLNames::styleAttr,
                      "width: 100px; height: 50px; background: red");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto* raster_invalidations =
      &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, raster_invalidations->size());
  EXPECT_EQ(IntRect(-43, 21, 107, 107), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, CompositedLayoutViewResize) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  EnableCompositing();
  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::classAttr, "");
  target->setAttribute(HTMLNames::styleAttr, "height: 2000px");
  GetDocument().View()->UpdateAllLifecyclePhases();

  // Resize the content.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "height: 3000px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto& raster_invalidations =
      GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, raster_invalidations.size());
  EXPECT_EQ(IntRect(0, 2000, 800, 1000), raster_invalidations[0].rect);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(&GetLayoutView()),
            raster_invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kBackgroundOnScrollingContentsLayer,
            raster_invalidations[0].reason);

  GetDocument().View()->SetTracksPaintInvalidations(false);

  // Resize the viewport. No paint invalidation.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  GetDocument().View()->Resize(800, 1000);
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(GetRasterInvalidationTracking()->HasInvalidations());
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, CompositedLayoutViewGradientResize) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  EnableCompositing();
  GetDocument().body()->setAttribute(HTMLNames::classAttr, "gradient");
  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::classAttr, "");
  target->setAttribute(HTMLNames::styleAttr, "height: 2000px");
  GetDocument().View()->UpdateAllLifecyclePhases();

  // Resize the content.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "height: 3000px");
  GetDocument().View()->UpdateAllLifecyclePhases();

  const auto& raster_invalidations =
      GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, raster_invalidations.size());
  EXPECT_EQ(IntRect(0, 0, 800, 3000), raster_invalidations[0].rect);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(&GetLayoutView()),
            raster_invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kBackgroundOnScrollingContentsLayer,
            raster_invalidations[0].reason);

  GetDocument().View()->SetTracksPaintInvalidations(false);

  // Resize the viewport. No paint invalidation.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  GetDocument().View()->Resize(800, 1000);
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(GetRasterInvalidationTracking()->HasInvalidations());
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, NonCompositedLayoutViewResize) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  SetBodyInnerHTML(R"HTML(
    <style>
      body { margin: 0 }
      iframe { display: block; width: 100px; height: 100px; border: none; }
    </style>
    <iframe id='iframe'></iframe>
  )HTML");
  SetChildFrameHTML(R"HTML(
    <style>
      ::-webkit-scrollbar { display: none }
      body { margin: 0; background: green; height: 0 }
    </style>
    <div id='content' style='width: 200px; height: 200px'></div>
  )HTML");
  GetDocument().View()->UpdateAllLifecyclePhases();
  Element* iframe = GetDocument().getElementById("iframe");
  Element* content = ChildDocument().getElementById("content");
  EXPECT_EQ(GetLayoutView(),
            content->GetLayoutObject()->ContainerForPaintInvalidation());

  // Resize the content.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  content->setAttribute(HTMLNames::styleAttr, "height: 500px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  // No invalidation because the changed part of layout overflow is clipped.
  EXPECT_FALSE(GetRasterInvalidationTracking()->HasInvalidations());
  GetDocument().View()->SetTracksPaintInvalidations(false);

  // Resize the iframe.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  iframe->setAttribute(HTMLNames::styleAttr, "height: 200px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto& raster_invalidations =
      GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations.size());
  EXPECT_EQ(IntRect(0, 100, 100, 100), raster_invalidations[0].rect);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(iframe->GetLayoutObject()),
            raster_invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[0].reason);
  EXPECT_EQ(
      static_cast<const DisplayItemClient*>(content->GetLayoutObject()->View()),
      raster_invalidations[1].client);
  EXPECT_EQ(IntRect(0, 100, 100, 100), raster_invalidations[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, NonCompositedLayoutViewGradientResize) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  SetBodyInnerHTML(R"HTML(
    <style>
      body { margin: 0 }
      iframe { display: block; width: 100px; height: 100px; border: none; }
    </style>
    <iframe id='iframe'></iframe>
  )HTML");
  SetChildFrameHTML(R"HTML(
    <style>
      ::-webkit-scrollbar { display: none }
      body {
        margin: 0;
        height: 0;
        background-image: linear-gradient(blue, yellow);
      }
    </style>
    <div id='content' style='width: 200px; height: 200px'></div>
  )HTML");
  GetDocument().View()->UpdateAllLifecyclePhases();
  Element* iframe = GetDocument().getElementById("iframe");
  Element* content = ChildDocument().getElementById("content");
  LayoutView* frame_layout_view = content->GetLayoutObject()->View();
  EXPECT_EQ(GetLayoutView(),
            content->GetLayoutObject()->ContainerForPaintInvalidation());

  // Resize the content.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  content->setAttribute(HTMLNames::styleAttr, "height: 500px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto* raster_invalidations =
      &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, raster_invalidations->size());
  EXPECT_EQ(IntRect(0, 0, 100, 100), (*raster_invalidations)[0].rect);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(frame_layout_view),
            (*raster_invalidations)[0].client);
  EXPECT_EQ(PaintInvalidationReason::kBackground,
            (*raster_invalidations)[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);

  // Resize the iframe.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  iframe->setAttribute(HTMLNames::styleAttr, "height: 200px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  raster_invalidations = &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations->size());
  EXPECT_EQ(IntRect(0, 100, 100, 100), (*raster_invalidations)[0].rect);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(iframe->GetLayoutObject()),
            (*raster_invalidations)[0].client);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            (*raster_invalidations)[0].reason);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(frame_layout_view),
            (*raster_invalidations)[1].client);
  EXPECT_EQ(IntRect(0, 0, 100, 200), (*raster_invalidations)[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, CompositedBackgroundAttachmentLocalResize) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  EnableCompositing();

  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::classAttr, "border local-background");
  target->setAttribute(HTMLNames::styleAttr, "will-change: transform");
  target->SetInnerHTMLFromString(
      "<div id=child style='width: 500px; height: 500px'></div>",
      ASSERT_NO_EXCEPTION);
  Element* child = GetDocument().getElementById("child");
  GetDocument().View()->UpdateAllLifecyclePhases();

  // Resize the content.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  child->setAttribute(HTMLNames::styleAttr, "width: 500px; height: 1000px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  LayoutBoxModelObject* target_obj =
      ToLayoutBoxModelObject(target->GetLayoutObject());
  GraphicsLayer* container_layer =
      target_obj->Layer()->GraphicsLayerBacking(target_obj);
  GraphicsLayer* contents_layer = target_obj->Layer()->GraphicsLayerBacking();
  // No invalidation on the container layer.
  EXPECT_FALSE(
      container_layer->GetRasterInvalidationTracking()->HasInvalidations());
  // Incremental invalidation of background on contents layer.
  const auto& contents_raster_invalidations =
      contents_layer->GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, contents_raster_invalidations.size());
  EXPECT_EQ(IntRect(0, 500, 500, 500), contents_raster_invalidations[0].rect);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(target->GetLayoutObject()),
            contents_raster_invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kBackgroundOnScrollingContentsLayer,
            contents_raster_invalidations[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);

  // Resize the container.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr,
                       "will-change: transform; height: 200px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  // No invalidation on the contents layer.
  EXPECT_FALSE(
      contents_layer->GetRasterInvalidationTracking()->HasInvalidations());
  // Incremental invalidation on the container layer.
  const auto& container_raster_invalidations =
      container_layer->GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, container_raster_invalidations.size());
  EXPECT_EQ(IntRect(0, 120, 70, 120), container_raster_invalidations[0].rect);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(target->GetLayoutObject()),
            container_raster_invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            container_raster_invalidations[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest,
       CompositedBackgroundAttachmentLocalGradientResize) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  EnableCompositing();

  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::classAttr,
                       "border local-background gradient");
  target->setAttribute(HTMLNames::styleAttr, "will-change: transform");
  target->SetInnerHTMLFromString(
      "<div id='child' style='width: 500px; height: 500px'></div>",
      ASSERT_NO_EXCEPTION);
  Element* child = GetDocument().getElementById("child");
  GetDocument().View()->UpdateAllLifecyclePhases();

  // Resize the content.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  child->setAttribute(HTMLNames::styleAttr, "width: 500px; height: 1000px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  LayoutBoxModelObject* target_obj =
      ToLayoutBoxModelObject(target->GetLayoutObject());
  GraphicsLayer* container_layer =
      target_obj->Layer()->GraphicsLayerBacking(target_obj);
  GraphicsLayer* contents_layer = target_obj->Layer()->GraphicsLayerBacking();
  // No invalidation on the container layer.
  EXPECT_FALSE(
      container_layer->GetRasterInvalidationTracking()->HasInvalidations());
  // Full invalidation of background on contents layer because the gradient
  // background is resized.
  const auto& contents_raster_invalidations =
      contents_layer->GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, contents_raster_invalidations.size());
  EXPECT_EQ(IntRect(0, 0, 500, 1000), contents_raster_invalidations[0].rect);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(target->GetLayoutObject()),
            contents_raster_invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kBackgroundOnScrollingContentsLayer,
            contents_raster_invalidations[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);

  // Resize the container.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr,
                       "will-change: transform; height: 200px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(
      contents_layer->GetRasterInvalidationTracking()->HasInvalidations());
  // Full invalidation on the container layer.
  const auto& container_raster_invalidations =
      container_layer->GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, container_raster_invalidations.size());
  EXPECT_EQ(IntRect(0, 0, 70, 240), container_raster_invalidations[0].rect);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(target->GetLayoutObject()),
            container_raster_invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            container_raster_invalidations[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, NonCompositedBackgroundAttachmentLocalResize) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::classAttr, "border local-background");
  target->SetInnerHTMLFromString(
      "<div id=child style='width: 500px; height: 500px'></div>",
      ASSERT_NO_EXCEPTION);
  Element* child = GetDocument().getElementById("child");
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_EQ(&GetLayoutView(),
            &target->GetLayoutObject()->ContainerForPaintInvalidation());

  // Resize the content.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  child->setAttribute(HTMLNames::styleAttr, "width: 500px; height: 1000px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  // No invalidation because the changed part is invisible.
  EXPECT_FALSE(GetRasterInvalidationTracking()->HasInvalidations());

  // Resize the container.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "height: 200px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto& raster_invalidations =
      GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, raster_invalidations.size());
  EXPECT_EQ(IntRect(0, 120, 70, 120), raster_invalidations[0].rect);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(target->GetLayoutObject()),
            raster_invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(BoxPaintInvalidatorTest, CompositedSolidBackgroundResize) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  EnableCompositing();
  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::classAttr, "solid-composited-scroller");
  target->SetInnerHTMLFromString("<div style='height: 500px'></div>",
                                 ASSERT_NO_EXCEPTION);
  GetDocument().View()->UpdateAllLifecyclePhases();

  // Resize the scroller.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "width: 100px");
  GetDocument().View()->UpdateAllLifecyclePhases();

  LayoutBoxModelObject* target_object =
      ToLayoutBoxModelObject(target->GetLayoutObject());
  GraphicsLayer* scrolling_contents_layer =
      target_object->Layer()->GraphicsLayerBacking();
  const auto& invalidations =
      scrolling_contents_layer->GetRasterInvalidationTracking()
          ->Invalidations();

  ASSERT_EQ(1u, invalidations.size());
  EXPECT_EQ(IntRect(50, 0, 50, 500), invalidations[0].rect);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(target_object),
            invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kBackgroundOnScrollingContentsLayer,
            invalidations[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

}  // namespace blink

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/paint_and_raster_invalidation_test.h"

#include "testing/gmock/include/gmock/gmock-matchers.h"

namespace blink {

void SetUpHTML(PaintAndRasterInvalidationTest& test) {
  test.SetBodyInnerHTML(R"HTML(
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
      .background {
        background: blue;
      }
      .solid-composited-scroller {
        overflow: scroll;
        will-change: transform;
        background: blue;
      }
      .local-background {
        background-attachment: local;
        overflow: scroll;
      }
      .gradient {
        background-image: linear-gradient(blue, yellow);
      }
      .transform {
        transform: scale(2);
      }
    </style>
    <div id='target' class='background'></div>
  )HTML");
}

INSTANTIATE_PAINT_TEST_CASE_P(PaintAndRasterInvalidationTest);

TEST_P(PaintAndRasterInvalidationTest, TrackingForTracing) {
  SetBodyInnerHTML(R"HTML(
    <style>#target { width: 100px; height: 100px; background: blue }</style>
    <div id="target"></div>
  )HTML");
  auto* target = GetDocument().getElementById("target");

  {
    // This is equivalent to enabling disabled-by-default-blink.invalidation
    // for tracing.
    ScopedPaintUnderInvalidationCheckingForTest checking(true);

    target->setAttribute(HTMLNames::styleAttr, "height: 200px");
    GetDocument().View()->UpdateAllLifecyclePhases();
    EXPECT_THAT(GetCcLayerClient()->TakeDebugInfo(GetCcLayer())->ToString(),
                testing::MatchesRegex(
                    "\\{\"layer_name\":.*\"annotated_invalidation_rects\":\\["
                    "\\{\"geometry_rect\":\\[8,108,100,100\\],"
                    "\"reason\":\"incremental\","
                    "\"client\":\"LayoutBlockFlow DIV id='target'\"\\}\\]\\}"));

    target->setAttribute(HTMLNames::styleAttr, "height: 200px; width: 200px");
    GetDocument().View()->UpdateAllLifecyclePhases();
    EXPECT_THAT(GetCcLayerClient()->TakeDebugInfo(GetCcLayer())->ToString(),
                testing::MatchesRegex(
                    "\\{\"layer_name\":.*\"annotated_invalidation_rects\":\\["
                    "\\{\"geometry_rect\":\\[108,8,100,200\\],"
                    "\"reason\":\"incremental\","
                    "\"client\":\"LayoutBlockFlow DIV id='target'\"\\}\\]\\}"));
  }

  target->setAttribute(HTMLNames::styleAttr, "height: 300px; width: 300px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_EQ(std::string::npos, GetCcLayerClient()
                                   ->TakeDebugInfo(GetCcLayer())
                                   ->ToString()
                                   .find("invalidation_rects"));
}

TEST_P(PaintAndRasterInvalidationTest, IncrementalInvalidationExpand) {
  SetUpHTML(*this);
  Element* target = GetDocument().getElementById("target");

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "width: 100px; height: 200px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto& raster_invalidations =
      GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations.size());
  EXPECT_EQ(IntRect(50, 0, 50, 200), raster_invalidations[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[0].reason);
  EXPECT_EQ(IntRect(0, 100, 100, 100), raster_invalidations[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest, IncrementalInvalidationShrink) {
  SetUpHTML(*this);
  Element* target = GetDocument().getElementById("target");

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "width: 20px; height: 80px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto& raster_invalidations =
      GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations.size());
  EXPECT_EQ(IntRect(20, 0, 30, 100), raster_invalidations[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[0].reason);
  EXPECT_EQ(IntRect(0, 80, 50, 20), raster_invalidations[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest, IncrementalInvalidationMixed) {
  SetUpHTML(*this);
  Element* target = GetDocument().getElementById("target");

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "width: 100px; height: 80px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto& raster_invalidations =
      GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations.size());
  EXPECT_EQ(IntRect(50, 0, 50, 80), raster_invalidations[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[0].reason);
  EXPECT_EQ(IntRect(0, 80, 50, 20), raster_invalidations[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest, SubpixelVisualRectChagne) {
  SetUpHTML(*this);
  Element* target = GetDocument().getElementById("target");

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "width: 100.6px; height: 70.3px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto* raster_invalidations =
      &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations->size());
  EXPECT_EQ(IntRect(0, 0, 50, 100), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[0].reason);
  EXPECT_EQ(IntRect(0, 0, 101, 71), (*raster_invalidations)[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "width: 50px; height: 100px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  raster_invalidations = &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations->size());
  EXPECT_EQ(IntRect(0, 0, 101, 71), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[0].reason);
  EXPECT_EQ(IntRect(0, 0, 50, 100), (*raster_invalidations)[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest, SubpixelVisualRectChangeWithTransform) {
  SetUpHTML(*this);
  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::classAttr, "background transform");
  GetDocument().View()->UpdateAllLifecyclePhases();

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "width: 100.6px; height: 70.3px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto* raster_invalidations =
      &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations->size());
  EXPECT_EQ(IntRect(0, 0, 100, 200), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[0].reason);
  EXPECT_EQ(IntRect(0, 0, 202, 142), (*raster_invalidations)[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr, "width: 50px; height: 100px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  raster_invalidations = &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(2u, raster_invalidations->size());
  EXPECT_EQ(IntRect(0, 0, 202, 142), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[0].reason);
  EXPECT_EQ(IntRect(0, 0, 100, 200), (*raster_invalidations)[1].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[1].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest, SubpixelWithinPixelsChange) {
  SetUpHTML(*this);
  Element* target = GetDocument().getElementById("target");
  LayoutObject* target_object = target->GetLayoutObject();
  EXPECT_EQ(LayoutRect(0, 0, 50, 100),
            target_object->FirstFragment().VisualRect());

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr,
                       "margin-top: 0.6px; width: 50px; height: 99.3px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_EQ(LayoutRect(0, 0, 50, 100),
            target_object->FirstFragment().VisualRect());
  const auto* raster_invalidations =
      &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, raster_invalidations->size());
  EXPECT_EQ(IntRect(0, 0, 50, 100), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr,
                       "margin-top: 0.6px; width: 49.3px; height: 98.5px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_EQ(LayoutRect(0, 0, 50, 100),
            target_object->FirstFragment().VisualRect());
  raster_invalidations = &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, raster_invalidations->size());
  EXPECT_EQ(IntRect(0, 0, 50, 100), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest, ResizeRotated) {
  SetUpHTML(*this);
  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::styleAttr, "transform: rotate(45deg)");
  GetDocument().View()->UpdateAllLifecyclePhases();

  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr,
                       "transform: rotate(45deg); width: 200px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto* raster_invalidations =
      &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, raster_invalidations->size());
  auto expected_rect = EnclosingIntRect(
      TransformationMatrix().Rotate(45).MapRect(FloatRect(50, 0, 150, 100)));
  expected_rect.Intersect(IntRect(0, 0, 800, 600));
  EXPECT_EQ(expected_rect, (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            (*raster_invalidations)[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest, ResizeRotatedChild) {
  SetUpHTML(*this);
  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::styleAttr,
                       "transform: rotate(45deg); width: 200px");
  target->SetInnerHTMLFromString(
      "<div id=child style='width: 50px; height: 50px; background: "
      "red'></div>");
  GetDocument().View()->UpdateAllLifecyclePhases();
  Element* child = GetDocument().getElementById("child");

  GetDocument().View()->SetTracksPaintInvalidations(true);
  child->setAttribute(HTMLNames::styleAttr,
                      "width: 100px; height: 50px; background: red");
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto* raster_invalidations =
      &GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, raster_invalidations->size());
  auto expected_rect = EnclosingIntRect(
      TransformationMatrix().Rotate(45).MapRect(FloatRect(50, 0, 50, 50)));
  expected_rect.Intersect(IntRect(0, 0, 800, 600));
  EXPECT_EQ(expected_rect, (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            (*raster_invalidations)[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest, CompositedLayoutViewResize) {
  // TODO(crbug.com/732611): Fix SPv2 for scrolling contents layer.
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  SetUpHTML(*this);
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
  EXPECT_EQ(static_cast<const DisplayItemClient*>(
                ViewScrollingContentsDisplayItemClient()),
            raster_invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[0].reason);

  GetDocument().View()->SetTracksPaintInvalidations(false);

  // Resize the viewport. No paint invalidation.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  GetDocument().View()->Resize(800, 1000);
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_FALSE(GetRasterInvalidationTracking()->HasInvalidations());
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest, CompositedLayoutViewGradientResize) {
  // TODO(crbug.com/732611): Fix SPv2 for scrolling contents layer.
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  SetUpHTML(*this);
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
  EXPECT_EQ(static_cast<const DisplayItemClient*>(
                ViewScrollingContentsDisplayItemClient()),
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

TEST_P(PaintAndRasterInvalidationTest, NonCompositedLayoutViewResize) {
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
  // The iframe doesn't have anything visible by itself, so we only issue raster
  // invalidation for the frame contents.
  const auto& raster_invalidations =
      GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, raster_invalidations.size());
  EXPECT_EQ(
      static_cast<const DisplayItemClient*>(content->GetLayoutObject()->View()),
      raster_invalidations[0].client);
  EXPECT_EQ(IntRect(0, 100, 100, 100), raster_invalidations[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest, NonCompositedLayoutViewGradientResize) {
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
  // The iframe doesn't have anything visible by itself, so we only issue raster
  // invalidation for the frame contents.
  ASSERT_EQ(1u, raster_invalidations->size());
  EXPECT_EQ(static_cast<const DisplayItemClient*>(frame_layout_view),
            (*raster_invalidations)[0].client);
  EXPECT_EQ(IntRect(0, 0, 100, 200), (*raster_invalidations)[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kGeometry,
            (*raster_invalidations)[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest,
       CompositedBackgroundAttachmentLocalResize) {
  // TODO(crbug.com/732611): Fix SPv2 for scrolling contents layer.
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  SetUpHTML(*this);
  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::classAttr, "background local-background");
  target->setAttribute(HTMLNames::styleAttr, "will-change: transform");
  target->SetInnerHTMLFromString(
      "<div id=child style='width: 500px; height: 500px'></div>",
      ASSERT_NO_EXCEPTION);
  Element* child = GetDocument().getElementById("child");
  GetDocument().View()->UpdateAllLifecyclePhases();

  auto* target_obj = ToLayoutBoxModelObject(target->GetLayoutObject());
  auto container_raster_invalidation_tracking =
      [&]() -> const RasterInvalidationTracking* {
    if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
      return GetRasterInvalidationTracking(1);
    return target_obj->Layer()
        ->GraphicsLayerBacking(target_obj)
        ->GetRasterInvalidationTracking();
  };
  auto contents_raster_invalidation_tracking =
      [&]() -> const RasterInvalidationTracking* {
    if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
      return GetRasterInvalidationTracking(2);
    return target_obj->Layer()
        ->GraphicsLayerBacking()
        ->GetRasterInvalidationTracking();
  };

  // Resize the content.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  child->setAttribute(HTMLNames::styleAttr, "width: 500px; height: 1000px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  // No invalidation on the container layer.
  EXPECT_FALSE(container_raster_invalidation_tracking()->HasInvalidations());
  // Incremental invalidation of background on contents layer.
  const auto& contents_raster_invalidations =
      contents_raster_invalidation_tracking()->Invalidations();
  ASSERT_EQ(1u, contents_raster_invalidations.size());
  EXPECT_EQ(IntRect(0, 500, 500, 500), contents_raster_invalidations[0].rect);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(
                target_obj->Layer()->GraphicsLayerBacking()),
            contents_raster_invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            contents_raster_invalidations[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);

  // Resize the container.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr,
                       "will-change: transform; height: 200px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  // No invalidation for composited layer resize.
  EXPECT_FALSE(container_raster_invalidation_tracking()->HasInvalidations());
  EXPECT_FALSE(contents_raster_invalidation_tracking()->HasInvalidations());
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest,
       CompositedBackgroundAttachmentLocalGradientResize) {
  // TODO(crbug.com/732611): Fix SPv2 for scrolling contents layer.
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  SetUpHTML(*this);
  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::classAttr, "local-background gradient");
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
  EXPECT_EQ(static_cast<const DisplayItemClient*>(contents_layer),
            contents_raster_invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kBackgroundOnScrollingContentsLayer,
            contents_raster_invalidations[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);

  // Resize the container.
  GetDocument().View()->SetTracksPaintInvalidations(true);
  target->setAttribute(HTMLNames::styleAttr,
                       "will-change: transform; height: 200px");
  GetDocument().View()->UpdateAllLifecyclePhases();
  // No explicit raster invalidation for composited layer resize.
  EXPECT_FALSE(
      container_layer->GetRasterInvalidationTracking()->HasInvalidations());
  EXPECT_FALSE(
      contents_layer->GetRasterInvalidationTracking()->HasInvalidations());
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest,
       NonCompositedBackgroundAttachmentLocalResize) {
  SetUpHTML(*this);
  Element* target = GetDocument().getElementById("target");
  target->setAttribute(HTMLNames::classAttr, "background local-background");
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
  EXPECT_EQ(IntRect(0, 100, 50, 100), raster_invalidations[0].rect);
  EXPECT_EQ(static_cast<const DisplayItemClient*>(target->GetLayoutObject()),
            raster_invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kIncremental,
            raster_invalidations[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest, CompositedSolidBackgroundResize) {
  // TODO(crbug.com/732611): Fix SPv2 for scrolling contents layer.
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  SetUpHTML(*this);
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
  EXPECT_EQ(static_cast<const DisplayItemClient*>(scrolling_contents_layer),
            invalidations[0].client);
  EXPECT_EQ(PaintInvalidationReason::kIncremental, invalidations[0].reason);
  GetDocument().View()->SetTracksPaintInvalidations(false);
}

// Changing style in a way that changes overflow without layout should cause
// the layout view to possibly need a paint invalidation since we may have
// revealed additional background that can be scrolled into view.
TEST_P(PaintAndRasterInvalidationTest, RecalcOverflowInvalidatesBackground) {
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

TEST_P(PaintAndRasterInvalidationTest,
       UpdateVisualRectOnFrameBorderWidthChange) {
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
  EXPECT_EQ(LayoutRect(0, 0, 100, 100),
            child_layout_view->FirstFragment().VisualRect());

  iframe->setAttribute(HTMLNames::styleAttr, "border: 20px solid blue");
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_EQ(GetDocument().GetLayoutView(),
            &child_layout_view->ContainerForPaintInvalidation());
  EXPECT_EQ(LayoutRect(0, 0, 100, 100),
            child_layout_view->FirstFragment().VisualRect());
};

TEST_P(PaintAndRasterInvalidationTest, DelayedFullPaintInvalidation) {
  // TODO(crbug.com/732611): Fix SPv2 for scrolling contents layer.
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

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
  EXPECT_FALSE(GetRasterInvalidationTracking()->HasInvalidations());
  EXPECT_EQ(PaintInvalidationReason::kDelayedFull,
            target->FullPaintInvalidationReason());
  EXPECT_FALSE(target->NeedsPaintOffsetAndVisualRectUpdate());
  GetDocument().View()->SetTracksPaintInvalidations(false);

  GetDocument().View()->SetTracksPaintInvalidations(true);
  // Scroll target into view.
  GetDocument().domWindow()->scrollTo(0, 4000);
  GetDocument().View()->UpdateAllLifecyclePhases();
  const auto& raster_invalidations =
      GetRasterInvalidationTracking()->Invalidations();
  ASSERT_EQ(1u, raster_invalidations.size());
  EXPECT_EQ(PaintInvalidationReason::kNone,
            target->FullPaintInvalidationReason());
  EXPECT_EQ(IntRect(0, 4000, 100, 100), raster_invalidations[0].rect);
  EXPECT_EQ(PaintInvalidationReason::kFull, raster_invalidations[0].reason);
  EXPECT_FALSE(target->NeedsPaintOffsetAndVisualRectUpdate());
  GetDocument().View()->SetTracksPaintInvalidations(false);
};

TEST_P(PaintAndRasterInvalidationTest, SVGHiddenContainer) {
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
  EXPECT_EQ(LayoutRect(55, 66, 7, 8), real_rect->FirstFragment().VisualRect());

  GetDocument().View()->SetTracksPaintInvalidations(true);
  ToElement(mask_rect->GetNode())->setAttribute("x", "20");
  GetDocument().View()->UpdateAllLifecyclePhasesExceptPaint();

  EXPECT_EQ(PaintInvalidationReason::kFull,
            real_rect->GetPaintInvalidationReason());
  // mask_rect has PaintInvalidationReason::kFull because it is not cached by
  // any PaintController.
  EXPECT_EQ(PaintInvalidationReason::kFull,
            mask_rect->GetPaintInvalidationReason());

  GetDocument().View()->UpdateAllLifecyclePhases();

  EXPECT_EQ(LayoutRect(), mask_rect->FirstFragment().VisualRect());
  EXPECT_EQ(LayoutRect(55, 66, 7, 8), real_rect->FirstFragment().VisualRect());

  // Should invalidate raster for real_rect only.
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
    // SPv2 creates composited layers for the rect and its mask.
    const auto& rect_raster_invalidations =
        GetRasterInvalidationTracking(1)->Invalidations();
    ASSERT_EQ(1u, rect_raster_invalidations.size());
    EXPECT_EQ(IntRect(0, 0, 7, 8), rect_raster_invalidations[0].rect);
    EXPECT_EQ(PaintInvalidationReason::kFull,
              rect_raster_invalidations[0].reason);
    const auto& mask_raster_invalidations =
        GetRasterInvalidationTracking(2)->Invalidations();
    ASSERT_EQ(1u, mask_raster_invalidations.size());
    EXPECT_EQ(IntRect(0, 0, 7, 8), mask_raster_invalidations[0].rect);
    EXPECT_EQ(PaintInvalidationReason::kFull,
              mask_raster_invalidations[0].reason);
  } else {
    const auto& raster_invalidations =
        GetRasterInvalidationTracking()->Invalidations();
    // The first is for the rect itself, the second is for its mask.
    ASSERT_EQ(2u, raster_invalidations.size());
    EXPECT_EQ(IntRect(155, 166, 7, 8), raster_invalidations[0].rect);
    EXPECT_EQ(PaintInvalidationReason::kFull, raster_invalidations[0].reason);
    EXPECT_EQ(IntRect(155, 166, 7, 8), raster_invalidations[1].rect);
    EXPECT_EQ(PaintInvalidationReason::kFull, raster_invalidations[1].reason);
  }

  GetDocument().View()->SetTracksPaintInvalidations(false);
}

TEST_P(PaintAndRasterInvalidationTest, UpdateVisualRectWhenPrinting) {
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

class PaintInvalidatorTestClient : public EmptyChromeClient {
 public:
  void InvalidateRect(const IntRect&) override {
    invalidation_recorded_ = true;
  }

  bool InvalidationRecorded() { return invalidation_recorded_; }

  void ResetInvalidationRecorded() { invalidation_recorded_ = false; }

 private:
  bool invalidation_recorded_ = false;
};

class PaintInvalidatorCustomClientTest : public RenderingTest {
 public:
  PaintInvalidatorCustomClientTest()
      : RenderingTest(EmptyLocalFrameClient::Create()),
        chrome_client_(new PaintInvalidatorTestClient) {}

  PaintInvalidatorTestClient& GetChromeClient() const override {
    return *chrome_client_;
  }

  bool InvalidationRecorded() { return chrome_client_->InvalidationRecorded(); }

  void ResetInvalidationRecorded() {
    chrome_client_->ResetInvalidationRecorded();
  }

 private:
  Persistent<PaintInvalidatorTestClient> chrome_client_;
};

TEST_F(PaintInvalidatorCustomClientTest,
       NonCompositedInvalidationChangeOpacity) {
  // This test runs in a non-composited mode, so invalidations should
  // be issued via InvalidateChromeClient.
  SetBodyInnerHTML(R"HTML(
    <div id=target style="opacity: 0.99"></div>
    )HTML");

  auto* target = GetDocument().getElementById("target");
  ASSERT_TRUE(target);

  ResetInvalidationRecorded();

  target->setAttribute(HTMLNames::styleAttr, "opacity: 0.98");
  GetDocument().View()->UpdateAllLifecyclePhases();

  EXPECT_TRUE(InvalidationRecorded());
}

}  // namespace blink

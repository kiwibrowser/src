// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/paint_controller_paint_test.h"

#include "cc/layers/picture_layer.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "third_party/blink/renderer/platform/graphics/compositing/paint_artifact_compositor.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/web_layer_tree_view_impl_for_testing.h"

namespace blink {

class RasterInvalidationTest : public PaintControllerPaintTest {
 protected:
  cc::Layer* GetCcLayer() const {
    if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
      return GetDocument()
          .View()
          ->GetPaintArtifactCompositorForTesting()
          ->RootLayer()
          ->child_at(0);
    }
    return GetLayoutView().Layer()->GraphicsLayerBacking()->ContentLayer();
  }

  cc::LayerClient* GetCcLayerClient() const {
    return GetCcLayer()->GetLayerClientForTesting();
  }

  void SetUp() override {
    PaintControllerPaintTest::SetUp();

    if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
      web_layer_tree_view_ = std::make_unique<WebLayerTreeViewImplForTesting>();
      web_layer_tree_view_->SetRootLayer(
          GetDocument()
              .View()
              ->GetPaintArtifactCompositorForTesting()
              ->RootLayer());
    }
  }

 private:
  std::unique_ptr<WebLayerTreeViewImplForTesting> web_layer_tree_view_;
};

INSTANTIATE_PAINT_TEST_CASE_P(RasterInvalidationTest);

TEST_P(RasterInvalidationTest, TrackingForTracing) {
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

}  // namespace blink

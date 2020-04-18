// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/compositing/paint_layer_compositor.h"

#include "third_party/blink/renderer/core/animation/animation.h"
#include "third_party/blink/renderer/core/animation/element_animation.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_layer_mapping.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"

namespace blink {

namespace {
class PaintLayerCompositorTest : public RenderingTest {
 public:
  PaintLayerCompositorTest()
      : RenderingTest(SingleChildLocalFrameClient::Create()) {}

 private:
  void SetUp() override {
    RenderingTest::SetUp();
    EnableCompositing();
  }
};
}  // namespace

TEST_F(PaintLayerCompositorTest, AdvancingToCompositingInputsClean) {
  SetBodyInnerHTML("<div id='box' style='position: relative'></div>");

  PaintLayer* box_layer =
      ToLayoutBox(GetLayoutObjectByElementId("box"))->Layer();
  ASSERT_TRUE(box_layer);
  EXPECT_FALSE(box_layer->NeedsCompositingInputsUpdate());

  box_layer->SetNeedsCompositingInputsUpdate();

  GetDocument().View()->UpdateLifecycleToCompositingInputsClean();
  EXPECT_EQ(DocumentLifecycle::kCompositingInputsClean,
            GetDocument().Lifecycle().GetState());
  EXPECT_FALSE(box_layer->NeedsCompositingInputsUpdate());

  GetDocument().View()->SetNeedsLayout();
  EXPECT_TRUE(GetDocument().View()->NeedsLayout());
}

TEST_F(PaintLayerCompositorTest,
       CompositingInputsCleanDoesNotTriggerAnimations) {
  SetBodyInnerHTML(R"HTML(
    <style>@keyframes fadeOut { from { opacity: 1; } to { opacity: 0; } }
    .animate { animation: fadeOut 2s; }</style>
    <div id='box'></div>
    <div id='otherBox'></div>
  )HTML");

  Element* box = GetDocument().getElementById("box");
  Element* otherBox = GetDocument().getElementById("otherBox");
  ASSERT_TRUE(box);
  ASSERT_TRUE(otherBox);

  box->setAttribute("class", "animate", ASSERT_NO_EXCEPTION);

  // Update the lifecycle to CompositingInputsClean. This should not start the
  // animation lifecycle.
  GetDocument().View()->UpdateLifecycleToCompositingInputsClean();
  EXPECT_EQ(DocumentLifecycle::kCompositingInputsClean,
            GetDocument().Lifecycle().GetState());

  otherBox->setAttribute("class", "animate", ASSERT_NO_EXCEPTION);

  // Now run the rest of the lifecycle. Because both 'box' and 'otherBox' were
  // given animations separated only by a lifecycle update to
  // CompositingInputsClean, they should both be started in the same lifecycle
  // and as such grouped together.
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_EQ(DocumentLifecycle::kPaintClean,
            GetDocument().Lifecycle().GetState());

  HeapVector<Member<Animation>> boxAnimations =
      ElementAnimation::getAnimations(*box);
  HeapVector<Member<Animation>> otherBoxAnimations =
      ElementAnimation::getAnimations(*box);

  EXPECT_EQ(1ul, boxAnimations.size());
  EXPECT_EQ(1ul, otherBoxAnimations.size());
  EXPECT_EQ(boxAnimations.front()->CompositorGroup(),
            otherBoxAnimations.front()->CompositorGroup());
}

TEST_F(PaintLayerCompositorTest, UpdateDoesNotOrphanMainGraphicsLayer) {
  SetHtmlInnerHTML(R"HTML(
    <style> * { margin: 0 } </style>
    <div id='box'></div>
  )HTML");

  auto* main_graphics_layer = GetDocument()
                                  .GetLayoutView()
                                  ->Layer()
                                  ->GetCompositedLayerMapping()
                                  ->MainGraphicsLayer();
  auto* main_graphics_layer_parent = main_graphics_layer->Parent();
  EXPECT_NE(nullptr, main_graphics_layer_parent);

  // Force CompositedLayerMapping to update the internal layer hierarchy.
  auto* box = GetDocument().getElementById("box");
  box->setAttribute(HTMLNames::styleAttr, "height: 1000px;");
  GetDocument().View()->UpdateAllLifecyclePhases();

  EXPECT_EQ(main_graphics_layer_parent, main_graphics_layer->Parent());
}

}  // namespace blink

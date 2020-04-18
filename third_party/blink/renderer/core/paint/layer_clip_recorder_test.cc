// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/layer_clip_recorder.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/compositing/paint_layer_compositor.h"
#include "third_party/blink/renderer/core/paint/paint_controller_paint_test.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"

namespace blink {
namespace {

class LayerClipRecorderTest : public PaintControllerPaintTestBase {
 private:
  void SetUp() override {
    PaintControllerPaintTestBase::SetUp();
    EnableCompositing();
  }
};

void DrawEmptyClip(GraphicsContext& context,
                   LayoutView& layout_view,
                   PaintPhase phase) {
  LayoutRect rect(1, 1, 9, 9);
  ClipRect clip_rect(rect);
  LayerClipRecorder layer_clip_recorder(
      context, *layout_view.Compositor()->RootLayer(),
      DisplayItem::kClipLayerForeground, clip_rect, nullptr, LayoutPoint(),
      PaintLayerFlags(),
      layout_view.Compositor()->RootLayer()->GetLayoutObject());
}

void DrawRectInClip(GraphicsContext& context,
                    LayoutView& layout_view,
                    PaintPhase phase) {
  IntRect rect(1, 1, 9, 9);
  ClipRect clip_rect((LayoutRect(rect)));
  LayerClipRecorder layer_clip_recorder(
      context, *layout_view.Compositor()->RootLayer(),
      DisplayItem::kClipLayerForeground, clip_rect, nullptr, LayoutPoint(),
      PaintLayerFlags(),
      layout_view.Compositor()->RootLayer()->GetLayoutObject());
  if (!DrawingRecorder::UseCachedDrawingIfPossible(context, layout_view,
                                                   phase)) {
    DrawingRecorder recorder(context, layout_view, phase);
    context.DrawRect(rect);
  }
}

TEST_F(LayerClipRecorderTest, Single) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  RootPaintController().InvalidateAll();
  GraphicsContext context(RootPaintController());
  EXPECT_EQ((size_t)0, RootPaintController().GetDisplayItemList().size());

  DrawRectInClip(context, GetLayoutView(), PaintPhase::kForeground);
  RootPaintController().CommitNewDisplayItems();
  EXPECT_EQ((size_t)3, RootPaintController().GetDisplayItemList().size());
  EXPECT_TRUE(DisplayItem::IsClipType(
      RootPaintController().GetDisplayItemList()[0].GetType()));
  EXPECT_TRUE(DisplayItem::IsDrawingType(
      RootPaintController().GetDisplayItemList()[1].GetType()));
  EXPECT_TRUE(DisplayItem::IsEndClipType(
      RootPaintController().GetDisplayItemList()[2].GetType()));
}

TEST_F(LayerClipRecorderTest, Empty) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;

  RootPaintController().InvalidateAll();
  GraphicsContext context(RootPaintController());
  EXPECT_EQ((size_t)0, RootPaintController().GetDisplayItemList().size());

  DrawEmptyClip(context, GetLayoutView(), PaintPhase::kForeground);
  RootPaintController().CommitNewDisplayItems();
  EXPECT_EQ((size_t)0, RootPaintController().GetDisplayItemList().size());
}

}  // namespace
}  // namespace blink

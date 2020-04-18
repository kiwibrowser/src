// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "build/build_config.h"
#include "cc/input/scrollbar.h"
#include "cc/layers/painted_overlay_scrollbar_layer.h"
#include "cc/layers/painted_scrollbar_layer.h"
#include "cc/layers/solid_color_layer.h"
#include "cc/paint/paint_canvas.h"
#include "cc/paint/paint_flags.h"
#include "cc/test/layer_tree_pixel_test.h"
#include "cc/test/test_in_process_context_provider.h"
#include "cc/trees/layer_tree_impl.h"
#include "gpu/command_buffer/client/gles2_interface.h"

#if !defined(OS_ANDROID)

namespace cc {
namespace {

class LayerTreeHostScrollbarsPixelTest : public LayerTreePixelTest {
 protected:
  LayerTreeHostScrollbarsPixelTest() = default;

  void InitializeSettings(LayerTreeSettings* settings) override {
    settings->layer_transforms_should_scale_layer_contents = true;
  }

  void SetupTree() override {
    SetInitialDeviceScaleFactor(device_scale_factor_);
    LayerTreePixelTest::SetupTree();
  }

  float device_scale_factor_ = 1.f;
};

class PaintedScrollbar : public Scrollbar {
 public:
  ~PaintedScrollbar() override = default;

  ScrollbarOrientation Orientation() const override { return VERTICAL; }
  bool IsLeftSideVerticalScrollbar() const override { return false; }
  gfx::Point Location() const override { return gfx::Point(); }
  bool IsOverlay() const override { return false; }
  bool HasThumb() const override { return thumb_; }
  int ThumbThickness() const override { return rect_.width(); }
  int ThumbLength() const override { return rect_.height(); }
  gfx::Rect TrackRect() const override { return rect_; }
  float ThumbOpacity() const override { return 1.f; }
  bool NeedsPaintPart(ScrollbarPart part) const override { return true; }
  bool HasTickmarks() const override { return false; }
  void PaintPart(PaintCanvas* canvas,
                 ScrollbarPart part,
                 const gfx::Rect& content_rect) override {
    PaintFlags flags;
    flags.setStyle(PaintFlags::kStroke_Style);
    flags.setStrokeWidth(SkIntToScalar(paint_scale_));
    flags.setColor(color_);

    gfx::Rect inset_rect = content_rect;
    while (!inset_rect.IsEmpty()) {
      int big = paint_scale_ + 2;
      int small = paint_scale_;
      inset_rect.Inset(big, big, small, small);
      canvas->drawRect(RectToSkRect(inset_rect), flags);
      inset_rect.Inset(big, big, small, small);
    }
  }
  bool UsesNinePatchThumbResource() const override { return false; }
  gfx::Size NinePatchThumbCanvasSize() const override { return gfx::Size(); }
  gfx::Rect NinePatchThumbAperture() const override { return gfx::Rect(); }

  void set_paint_scale(int scale) { paint_scale_ = scale; }

 private:
  int paint_scale_ = 4;
  bool thumb_ = false;
  SkColor color_ = SK_ColorGREEN;
  gfx::Rect rect_;
};

TEST_F(LayerTreeHostScrollbarsPixelTest, NoScale) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorWHITE);

  auto scrollbar = std::make_unique<PaintedScrollbar>();
  scoped_refptr<PaintedScrollbarLayer> layer =
      PaintedScrollbarLayer::Create(std::move(scrollbar));
  layer->SetIsDrawable(true);
  layer->SetBounds(gfx::Size(200, 200));
  background->AddChild(layer);

  RunPixelTest(PIXEL_TEST_GL, background,
               base::FilePath(FILE_PATH_LITERAL("spiral.png")));
}

TEST_F(LayerTreeHostScrollbarsPixelTest, DeviceScaleFactor) {
  // With a device scale of 2, the scrollbar should still be rendered
  // pixel-perfect, not show scaling artifacts
  device_scale_factor_ = 2.f;

  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(100, 100), SK_ColorWHITE);

  auto scrollbar = std::make_unique<PaintedScrollbar>();
  scoped_refptr<PaintedScrollbarLayer> layer =
      PaintedScrollbarLayer::Create(std::move(scrollbar));
  layer->SetIsDrawable(true);
  layer->SetBounds(gfx::Size(100, 100));
  background->AddChild(layer);

  RunPixelTest(PIXEL_TEST_GL, background,
               base::FilePath(FILE_PATH_LITERAL("spiral_double_scale.png")));
}

TEST_F(LayerTreeHostScrollbarsPixelTest, TransformScale) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(200, 200), SK_ColorWHITE);

  auto scrollbar = std::make_unique<PaintedScrollbar>();
  scoped_refptr<PaintedScrollbarLayer> layer =
      PaintedScrollbarLayer::Create(std::move(scrollbar));
  layer->SetIsDrawable(true);
  layer->SetBounds(gfx::Size(100, 100));
  background->AddChild(layer);

  // This has a scale of 2, it should still be rendered pixel-perfect, not show
  // scaling artifacts
  gfx::Transform scale_transform;
  scale_transform.Scale(2.0, 2.0);
  layer->SetTransform(scale_transform);

  RunPixelTest(PIXEL_TEST_GL, background,
               base::FilePath(FILE_PATH_LITERAL("spiral_double_scale.png")));
}

TEST_F(LayerTreeHostScrollbarsPixelTest, HugeTransformScale) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(400, 400), SK_ColorWHITE);

  auto scrollbar = std::make_unique<PaintedScrollbar>();
  scrollbar->set_paint_scale(1);
  scoped_refptr<PaintedScrollbarLayer> layer =
      PaintedScrollbarLayer::Create(std::move(scrollbar));
  layer->SetIsDrawable(true);
  layer->SetBounds(gfx::Size(10, 400));
  background->AddChild(layer);

  scoped_refptr<TestInProcessContextProvider> context(
      new TestInProcessContextProvider(/*enable_oop_rasterization=*/false));
  context->BindToCurrentThread();
  int max_texture_size = 0;
  context->ContextGL()->GetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

  // We want a scale that creates a texture taller than the max texture size. If
  // there's no clamping, the texture will be invalid and we'll just get black.
  double scale = 64.0;
  ASSERT_GT(scale * layer->bounds().height(), max_texture_size);

  // Let's show the bottom right of the layer, so we know the texture wasn't
  // just cut off.
  layer->SetPosition(
      gfx::PointF(-10.f * scale + 400.f, -400.f * scale + 400.f));

  gfx::Transform scale_transform;
  scale_transform.Scale(scale, scale);
  layer->SetTransform(scale_transform);

  RunPixelTest(PIXEL_TEST_GL, background,
               base::FilePath(FILE_PATH_LITERAL("spiral_64_scale.png")));
}

class LayerTreeHostOverlayScrollbarsPixelTest
    : public LayerTreeHostScrollbarsPixelTest {
 protected:
  LayerTreeHostOverlayScrollbarsPixelTest() = default;

  void DidActivateTreeOnThread(LayerTreeHostImpl* host_impl) override {
    LayerImpl* layer = host_impl->active_tree()->LayerById(scrollbar_layer_id_);
    ScrollbarLayerImplBase* scrollbar = layer->ToScrollbarLayer();
    scrollbar->SetThumbThicknessScaleFactor(thickness_scale_);
  }

  int scrollbar_layer_id_;
  float thickness_scale_;
};

class PaintedOverlayScrollbar : public PaintedScrollbar {
 public:
  ~PaintedOverlayScrollbar() override = default;

  int ThumbThickness() const override { return 15; }
  int ThumbLength() const override { return 50; }
  gfx::Rect TrackRect() const override { return gfx::Rect(0, 0, 15, 400); }
  bool HasThumb() const override { return true; }
  bool IsOverlay() const override { return true; }
  void PaintPart(PaintCanvas* canvas,
                 ScrollbarPart part,
                 const gfx::Rect& content_rect) override {
    // The outside of the rect will be painted with a 1 pixel black, red, then
    // blue border. The inside will be solid blue. This will allow the test to
    // ensure that scaling the thumb doesn't scale the border at all.  Note
    // that the inside of the border must be the same color as the center tile
    // to prevent an interpolation from being applied.
    PaintFlags flags;
    flags.setStyle(PaintFlags::kFill_Style);
    flags.setStrokeWidth(SkIntToScalar(1));
    flags.setColor(SK_ColorBLACK);

    gfx::Rect inset_rect = content_rect;

    canvas->drawRect(RectToSkRect(inset_rect), flags);

    flags.setColor(SK_ColorRED);
    inset_rect.Inset(1, 1);
    canvas->drawRect(RectToSkRect(inset_rect), flags);

    flags.setColor(SK_ColorBLUE);
    inset_rect.Inset(1, 1);
    canvas->drawRect(RectToSkRect(inset_rect), flags);
  }
  bool UsesNinePatchThumbResource() const override { return true; }
  gfx::Size NinePatchThumbCanvasSize() const override {
    return gfx::Size(7, 7);
  }
  gfx::Rect NinePatchThumbAperture() const override {
    return gfx::Rect(3, 3, 1, 1);
  }
};

// Simulate increasing the thickness of a painted overlay scrollbar. Ensure that
// the scrollbar border remains crisp.
TEST_F(LayerTreeHostOverlayScrollbarsPixelTest, NinePatchScrollbarScaledUp) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(400, 400), SK_ColorWHITE);

  auto scrollbar = std::make_unique<PaintedOverlayScrollbar>();
  scoped_refptr<PaintedOverlayScrollbarLayer> layer =
      PaintedOverlayScrollbarLayer::Create(std::move(scrollbar));

  scrollbar_layer_id_ = layer->id();
  thickness_scale_ = 5.f;

  layer->SetIsDrawable(true);
  layer->SetBounds(gfx::Size(10, 300));
  background->AddChild(layer);

  layer->SetPosition(gfx::PointF(185, 10));

  RunPixelTest(
      PIXEL_TEST_GL, background,
      base::FilePath(FILE_PATH_LITERAL("overlay_scrollbar_scaled_up.png")));
}

// Simulate decreasing the thickness of a painted overlay scrollbar. Ensure that
// the scrollbar border remains crisp.
TEST_F(LayerTreeHostOverlayScrollbarsPixelTest, NinePatchScrollbarScaledDown) {
  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(400, 400), SK_ColorWHITE);

  auto scrollbar = std::make_unique<PaintedOverlayScrollbar>();
  scoped_refptr<PaintedOverlayScrollbarLayer> layer =
      PaintedOverlayScrollbarLayer::Create(std::move(scrollbar));

  scrollbar_layer_id_ = layer->id();
  thickness_scale_ = 0.4f;

  layer->SetIsDrawable(true);
  layer->SetBounds(gfx::Size(10, 300));
  background->AddChild(layer);

  layer->SetPosition(gfx::PointF(185, 10));

  RunPixelTest(
      PIXEL_TEST_GL, background,
      base::FilePath(FILE_PATH_LITERAL("overlay_scrollbar_scaled_down.png")));
}

}  // namespace
}  // namespace cc

#endif  // OS_ANDROID

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "cc/layers/picture_image_layer.h"
#include "cc/layers/solid_color_layer.h"
#include "cc/paint/paint_image.h"
#include "cc/paint/paint_image_builder.h"
#include "cc/test/layer_tree_pixel_resource_test.h"
#include "cc/test/pixel_comparator.h"
#include "components/viz/test/test_layer_tree_frame_sink.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSurface.h"

#if !defined(OS_ANDROID)

namespace cc {
namespace {

SkBlendMode const kBlendModes[] = {
    SkBlendMode::kSrcOver,   SkBlendMode::kScreen,
    SkBlendMode::kOverlay,   SkBlendMode::kDarken,
    SkBlendMode::kLighten,   SkBlendMode::kColorDodge,
    SkBlendMode::kColorBurn, SkBlendMode::kHardLight,
    SkBlendMode::kSoftLight, SkBlendMode::kDifference,
    SkBlendMode::kExclusion, SkBlendMode::kMultiply,
    SkBlendMode::kHue,       SkBlendMode::kSaturation,
    SkBlendMode::kColor,     SkBlendMode::kLuminosity};

SkColor kCSSTestColors[] = {
    0xffff0000,  // red
    0xff00ff00,  // lime
    0xff0000ff,  // blue
    0xff00ffff,  // aqua
    0xffff00ff,  // fuchsia
    0xffffff00,  // yellow
    0xff008000,  // green
    0xff800000,  // maroon
    0xff000080,  // navy
    0xff800080,  // purple
    0xff808000,  // olive
    0xff008080,  // teal
    0xfffa8072,  // salmon
    0xffc0c0c0,  // silver
    0xff000000,  // black
    0xff808080,  // gray
    0x80000000,  // black with transparency
    0xffffffff,  // white
    0x80ffffff,  // white with transparency
    0x00000000   // transparent
};

const int kBlendModesCount = arraysize(kBlendModes);
const int kCSSTestColorsCount = arraysize(kCSSTestColors);

using RenderPassOptions = uint32_t;
const uint32_t kUseMasks = 1 << 0;
const uint32_t kUseAntialiasing = 1 << 1;
const uint32_t kUseColorMatrix = 1 << 2;
const uint32_t kForceShaders = 1 << 3;

class LayerTreeHostBlendingPixelTest : public LayerTreeHostPixelResourceTest {
 public:
  LayerTreeHostBlendingPixelTest()
      : force_antialiasing_(false), force_blending_with_shaders_(false) {
    pixel_comparator_.reset(new FuzzyPixelOffByOneComparator(true));
  }

 protected:
  std::unique_ptr<viz::TestLayerTreeFrameSink> CreateLayerTreeFrameSink(
      const viz::RendererSettings& renderer_settings,
      double refresh_rate,
      scoped_refptr<viz::ContextProvider> compositor_context_provider,
      scoped_refptr<viz::RasterContextProvider> worker_context_provider)
      override {
    viz::RendererSettings modified_renderer_settings = renderer_settings;
    modified_renderer_settings.force_antialiasing = force_antialiasing_;
    modified_renderer_settings.force_blending_with_shaders =
        force_blending_with_shaders_;
    return LayerTreeHostPixelResourceTest::CreateLayerTreeFrameSink(
        modified_renderer_settings, refresh_rate, compositor_context_provider,
        worker_context_provider);
  }

  void RunBlendingWithRootPixelTestType(PixelResourceTestCase type) {
    const int kLaneWidth = 2;
    const int kLaneHeight = kLaneWidth;
    const int kRootWidth = (kBlendModesCount + 2) * kLaneWidth;
    const int kRootHeight = 2 * kLaneWidth + kLaneHeight;
    InitializeFromTestCase(type);

    scoped_refptr<SolidColorLayer> background =
        CreateSolidColorLayer(gfx::Rect(kRootWidth, kRootHeight), kCSSOrange);

    // Orange child layers will blend with the green background
    for (int i = 0; i < kBlendModesCount; ++i) {
      gfx::Rect child_rect(
          (i + 1) * kLaneWidth, kLaneWidth, kLaneWidth, kLaneHeight);
      scoped_refptr<SolidColorLayer> green_lane =
          CreateSolidColorLayer(child_rect, kCSSGreen);
      background->AddChild(green_lane);
      green_lane->SetBlendMode(kBlendModes[i]);
    }

    RunPixelResourceTest(
        background,
        base::FilePath(FILE_PATH_LITERAL("blending_with_root.png")));
  }

  void RunBlendingWithTransparentPixelTestType(PixelResourceTestCase type) {
    const int kLaneWidth = 2;
    const int kLaneHeight = 3 * kLaneWidth;
    const int kRootWidth = (kBlendModesCount + 2) * kLaneWidth;
    const int kRootHeight = 2 * kLaneWidth + kLaneHeight;
    InitializeFromTestCase(type);

    scoped_refptr<SolidColorLayer> root =
        CreateSolidColorLayer(gfx::Rect(kRootWidth, kRootHeight), kCSSBrown);

    scoped_refptr<SolidColorLayer> background = CreateSolidColorLayer(
        gfx::Rect(0, kLaneWidth * 2, kRootWidth, kLaneWidth), kCSSOrange);

    root->AddChild(background);
    background->SetIsRootForIsolatedGroup(true);

    // Orange child layers will blend with the green background
    for (int i = 0; i < kBlendModesCount; ++i) {
      gfx::Rect child_rect(
          (i + 1) * kLaneWidth, -kLaneWidth, kLaneWidth, kLaneHeight);
      scoped_refptr<SolidColorLayer> green_lane =
          CreateSolidColorLayer(child_rect, kCSSGreen);
      background->AddChild(green_lane);
      green_lane->SetBlendMode(kBlendModes[i]);
    }

    RunPixelResourceTest(
        root, base::FilePath(FILE_PATH_LITERAL("blending_transparent.png")));
  }

  scoped_refptr<Layer> CreateColorfulBackdropLayer(int width, int height) {
    // Draw the backdrop with horizontal lanes.
    const int kLaneWidth = width;
    const int kLaneHeight = height / kCSSTestColorsCount;
    sk_sp<SkSurface> backing_store =
        SkSurface::MakeRasterN32Premul(width, height);
    SkCanvas* canvas = backing_store->getCanvas();
    canvas->clear(SK_ColorTRANSPARENT);
    for (int i = 0; i < kCSSTestColorsCount; ++i) {
      SkPaint paint;
      paint.setColor(kCSSTestColors[i]);
      canvas->drawRect(
          SkRect::MakeXYWH(0, i * kLaneHeight, kLaneWidth, kLaneHeight), paint);
    }
    scoped_refptr<PictureImageLayer> layer = PictureImageLayer::Create();
    layer->SetIsDrawable(true);
    layer->SetBounds(gfx::Size(width, height));
    layer->SetImage(PaintImageBuilder::WithDefault()
                        .set_id(PaintImage::GetNextId())
                        .set_image(backing_store->makeImageSnapshot(),
                                   PaintImage::GetNextContentId())
                        .TakePaintImage(),
                    SkMatrix::I(), false);
    return layer;
  }

  void SetupMaskLayer(scoped_refptr<Layer> layer) {
    const int kMaskOffset = 2;
    gfx::Size bounds = layer->bounds();
    scoped_refptr<PictureImageLayer> mask = PictureImageLayer::Create();
    mask->SetIsDrawable(true);
    mask->SetLayerMaskType(Layer::LayerMaskType::MULTI_TEXTURE_MASK);
    mask->SetBounds(bounds);

    sk_sp<SkSurface> surface =
        SkSurface::MakeRasterN32Premul(bounds.width(), bounds.height());
    SkCanvas* canvas = surface->getCanvas();
    SkPaint paint;
    paint.setColor(SK_ColorWHITE);
    canvas->clear(SK_ColorTRANSPARENT);
    canvas->drawRect(SkRect::MakeXYWH(kMaskOffset, kMaskOffset,
                                      bounds.width() - kMaskOffset * 2,
                                      bounds.height() - kMaskOffset * 2),
                     paint);
    mask->SetImage(PaintImageBuilder::WithDefault()
                       .set_id(PaintImage::GetNextId())
                       .set_image(surface->makeImageSnapshot(),
                                  PaintImage::GetNextContentId())
                       .TakePaintImage(),
                   SkMatrix::I(), false);
    layer->SetMaskLayer(mask.get());
  }

  void SetupColorMatrix(scoped_refptr<Layer> layer) {
    FilterOperations filter_operations;
    filter_operations.Append(FilterOperation::CreateSepiaFilter(.001f));
    layer->SetFilters(filter_operations);
  }

  void CreateBlendingColorLayers(int lane_width,
                                 int lane_height,
                                 scoped_refptr<Layer> background,
                                 RenderPassOptions flags) {
    const int kLanesCount = kBlendModesCount + 4;
    const SkColor kMiscOpaqueColor = 0xffc86464;
    const SkColor kMiscTransparentColor = 0x80c86464;
    const SkBlendMode kCoeffBlendMode = SkBlendMode::kScreen;
    const SkBlendMode kShaderBlendMode = SkBlendMode::kColorBurn;
    // add vertical lanes with each of the blend modes
    for (int i = 0; i < kLanesCount; ++i) {
      gfx::Rect child_rect(i * lane_width, 0, lane_width, lane_height);
      SkBlendMode blend_mode = SkBlendMode::kSrcOver;
      float opacity = 1.f;
      SkColor color = kMiscOpaqueColor;

      if (i < kBlendModesCount) {
        blend_mode = kBlendModes[i];
      } else if (i == kBlendModesCount) {
        blend_mode = kCoeffBlendMode;
        opacity = 0.5f;
      } else if (i == kBlendModesCount + 1) {
        blend_mode = kCoeffBlendMode;
        color = kMiscTransparentColor;
      } else if (i == kBlendModesCount + 2) {
        blend_mode = kShaderBlendMode;
        opacity = 0.5f;
      } else if (i == kBlendModesCount + 3) {
        blend_mode = kShaderBlendMode;
        color = kMiscTransparentColor;
      }
      scoped_refptr<SolidColorLayer> lane =
          CreateSolidColorLayer(child_rect, color);
      lane->SetBlendMode(blend_mode);
      lane->SetOpacity(opacity);
      lane->SetForceRenderSurfaceForTesting(true);
      // Layers with kDstIn blend mode with a mask is not supported.
      if (flags & kUseMasks)
        SetupMaskLayer(lane);
      if (flags & kUseColorMatrix) {
        SetupColorMatrix(lane);
      }
      background->AddChild(lane);
    }
  }

  void RunBlendingWithRenderPass(PixelResourceTestCase type,
                                 const base::FilePath::CharType* expected_path,
                                 RenderPassOptions flags) {
    const int kLaneWidth = 8;
    const int kRootWidth = kLaneWidth * (kBlendModesCount + 4);
    const int kRootHeight = kLaneWidth * kCSSTestColorsCount;
    InitializeFromTestCase(type);

    scoped_refptr<SolidColorLayer> root = CreateSolidColorLayer(
        gfx::Rect(kRootWidth, kRootHeight), SK_ColorWHITE);
    scoped_refptr<Layer> background =
        CreateColorfulBackdropLayer(kRootWidth, kRootHeight);

    background->SetIsRootForIsolatedGroup(true);
    root->AddChild(background);

    CreateBlendingColorLayers(kLaneWidth, kRootHeight, background.get(), flags);

    this->force_antialiasing_ = (flags & kUseAntialiasing);
    this->force_blending_with_shaders_ = (flags & kForceShaders);

    if ((flags & kUseAntialiasing) && (test_type_ == PIXEL_TEST_GL)) {
      // Anti aliasing causes differences up to 8 pixels at the edges.
      int large_error_allowed = 8;
      // Blending results might differ with one pixel.
      int small_error_allowed = 1;
      // Most of the errors are one pixel errors.
      float percentage_pixels_small_error = 13.2f;
      // Because of anti-aliasing, around 10% of pixels (at the edges) have
      // bigger errors (from small_error_allowed + 1 to large_error_allowed).
      float percentage_pixels_error = 22.5f;
      // The average error is still close to 1.
      float average_error_allowed_in_bad_pixels = 1.4f;

      pixel_comparator_.reset(
          new FuzzyPixelComparator(false,  // discard_alpha
                                   percentage_pixels_error,
                                   percentage_pixels_small_error,
                                   average_error_allowed_in_bad_pixels,
                                   large_error_allowed,
                                   small_error_allowed));
    }

    RunPixelResourceTest(root, base::FilePath(expected_path));
  }

  bool force_antialiasing_;
  bool force_blending_with_shaders_;
};

TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithRoot_GL) {
  RunBlendingWithRootPixelTestType(ZERO_COPY);
}

TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithRoot_Software) {
  RunBlendingWithRootPixelTestType(SOFTWARE);
}

TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithBackgroundFilter) {
  const int kLaneWidth = 2;
  const int kLaneHeight = kLaneWidth;
  const int kRootWidth = (kBlendModesCount + 2) * kLaneWidth;
  const int kRootHeight = 2 * kLaneWidth + kLaneHeight;
  InitializeFromTestCase(ZERO_COPY);

  scoped_refptr<SolidColorLayer> background =
      CreateSolidColorLayer(gfx::Rect(kRootWidth, kRootHeight), kCSSOrange);

  // Orange child layers have a background filter set and they will blend with
  // the green background
  for (int i = 0; i < kBlendModesCount; ++i) {
    gfx::Rect child_rect(
        (i + 1) * kLaneWidth, kLaneWidth, kLaneWidth, kLaneHeight);
    scoped_refptr<SolidColorLayer> green_lane =
        CreateSolidColorLayer(child_rect, kCSSGreen);
    background->AddChild(green_lane);

    FilterOperations filters;
    filters.Append(FilterOperation::CreateGrayscaleFilter(.75));
    green_lane->SetBackgroundFilters(filters);
    green_lane->SetBlendMode(kBlendModes[i]);
  }

  RunPixelResourceTest(
      background, base::FilePath(FILE_PATH_LITERAL("blending_and_filter.png")));
}

TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithTransparent_GL) {
  RunBlendingWithTransparentPixelTestType(ZERO_COPY);
}

TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithTransparent_Software) {
  RunBlendingWithTransparentPixelTestType(SOFTWARE);
}

// Tests for render passes
TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithRenderPass_GL) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass.png"), 0);
}

TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithRenderPass_Software) {
  RunBlendingWithRenderPass(SOFTWARE,
                            FILE_PATH_LITERAL("blending_render_pass.png"), 0);
}

TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithRenderPassAA_GL) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass.png"),
                            kUseAntialiasing);
}

TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithRenderPassAA_Software) {
  RunBlendingWithRenderPass(SOFTWARE,
                            FILE_PATH_LITERAL("blending_render_pass.png"),
                            kUseAntialiasing);
}

TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithRenderPassWithMask_GL) {
  RunBlendingWithRenderPass(
      ZERO_COPY, FILE_PATH_LITERAL("blending_render_pass_mask.png"), kUseMasks);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassWithMask_Software) {
  RunBlendingWithRenderPass(
      SOFTWARE, FILE_PATH_LITERAL("blending_render_pass_mask.png"), kUseMasks);
}

TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithRenderPassWithMaskAA_GL) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass_mask.png"),
                            kUseMasks | kUseAntialiasing);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassWithMaskAA_Software) {
  RunBlendingWithRenderPass(SOFTWARE,
                            FILE_PATH_LITERAL("blending_render_pass_mask.png"),
                            kUseMasks | kUseAntialiasing);
}

TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithRenderPassColorMatrix_GL) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass.png"),
                            kUseColorMatrix);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassColorMatrix_Software) {
  RunBlendingWithRenderPass(
      SOFTWARE, FILE_PATH_LITERAL("blending_render_pass.png"), kUseColorMatrix);
}

TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithRenderPassColorMatrixAA_GL) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass.png"),
                            kUseAntialiasing | kUseColorMatrix);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassColorMatrixAA_Software) {
  RunBlendingWithRenderPass(SOFTWARE,
                            FILE_PATH_LITERAL("blending_render_pass.png"),
                            kUseAntialiasing | kUseColorMatrix);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassWithMaskColorMatrix_GL) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass_mask.png"),
                            kUseMasks | kUseColorMatrix);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassWithMaskColorMatrix_Software) {
  RunBlendingWithRenderPass(SOFTWARE,
                            FILE_PATH_LITERAL("blending_render_pass_mask.png"),
                            kUseMasks | kUseColorMatrix);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassWithMaskColorMatrixAA_GL) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass_mask.png"),
                            kUseMasks | kUseAntialiasing | kUseColorMatrix);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassWithMaskColorMatrixAA_Software) {
  RunBlendingWithRenderPass(SOFTWARE,
                            FILE_PATH_LITERAL("blending_render_pass_mask.png"),
                            kUseMasks | kUseAntialiasing | kUseColorMatrix);
}

// Tests for render passes forcing shaders for all the blend modes.
TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithRenderPassShaders_GL) {
  RunBlendingWithRenderPass(
      ZERO_COPY, FILE_PATH_LITERAL("blending_render_pass.png"), kForceShaders);
}

TEST_F(LayerTreeHostBlendingPixelTest, BlendingWithRenderPassShadersAA_GL) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass.png"),
                            kUseAntialiasing | kForceShaders);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassShadersWithMask_GL) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass_mask.png"),
                            kUseMasks | kForceShaders);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassShadersWithMask_GL_TextureRect) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass_mask.png"),
                            kUseMasks | kForceShaders);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassShadersWithMaskAA_GL) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass_mask.png"),
                            kUseMasks | kUseAntialiasing | kForceShaders);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassShadersWithMaskAA_GL_TextureRect) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass_mask.png"),
                            kUseMasks | kUseAntialiasing | kForceShaders);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassShadersColorMatrix_GL) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass.png"),
                            kUseColorMatrix | kForceShaders);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassShadersColorMatrixAA_GL) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass.png"),
                            kUseAntialiasing | kUseColorMatrix | kForceShaders);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassShadersWithMaskColorMatrix_GL) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass_mask.png"),
                            kUseMasks | kUseColorMatrix | kForceShaders);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassShadersWithMaskColorMatrix_GL_TextureRect) {
  RunBlendingWithRenderPass(ZERO_COPY,
                            FILE_PATH_LITERAL("blending_render_pass_mask.png"),
                            kUseMasks | kUseColorMatrix | kForceShaders);
}

TEST_F(LayerTreeHostBlendingPixelTest,
       BlendingWithRenderPassShadersWithMaskColorMatrixAA_GL) {
  RunBlendingWithRenderPass(
      ZERO_COPY, FILE_PATH_LITERAL("blending_render_pass_mask.png"),
      kUseMasks | kUseAntialiasing | kUseColorMatrix | kForceShaders);
}

}  // namespace
}  // namespace cc

#endif  // OS_ANDROID

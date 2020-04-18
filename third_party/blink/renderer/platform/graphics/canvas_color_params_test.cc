// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/canvas_color_params.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/graphics/color_correction_test_utils.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/skia/include/core/SkColorSpaceXform.h"
#include "ui/gfx/color_space.h"

namespace blink {

// When drawing a color managed canvas, the target SkColorSpace is obtained by
// calling CanvasColorParams::GetSkColorSpaceForSkSurfaces(). When drawing media
// to the canvas, the target gfx::ColorSpace is returned by CanvasColorParams::
// GetStorageGfxColorSpace(). This test verifies that the two different color
// spaces are approximately the same for different CanvasColorParam objects.
// This test does not use SkColorSpace::Equals() since it is sensitive to
// rounding issues (floats don't round-trip perfectly through ICC fixed point).
// Instead, it color converts a pixel and compares the result. Furthermore, this
// test does not include sRGB as target color space since we use
// SkColorSpaceXformCanvas for sRGB targets and GetSkColorSpaceForSkSurfaces()
// returns nullptr for the surface.
TEST(CanvasColorParamsTest, MatchSkColorSpaceWithGfxColorSpace) {
  sk_sp<SkColorSpace> src_rgb_color_space = SkColorSpace::MakeSRGB();
  std::unique_ptr<uint8_t[]> src_pixel(new uint8_t[4]{32, 96, 160, 255});

  CanvasColorSpace canvas_color_spaces[] = {
      kSRGBCanvasColorSpace, kRec2020CanvasColorSpace, kP3CanvasColorSpace,
  };

  for (int iter_color_space = 0; iter_color_space < 3; iter_color_space++) {
    CanvasColorParams color_params(canvas_color_spaces[iter_color_space],
                                   kF16CanvasPixelFormat, kNonOpaque);

    std::unique_ptr<SkColorSpaceXform> color_space_xform_canvas =
        SkColorSpaceXform::New(src_rgb_color_space.get(),
                               color_params.GetSkColorSpace().get());
    std::unique_ptr<SkColorSpaceXform> color_space_xform_media =
        SkColorSpaceXform::New(
            src_rgb_color_space.get(),
            color_params.GetStorageGfxColorSpace().ToSkColorSpace().get());

    std::unique_ptr<uint8_t[]> transformed_pixel_canvas(
        new uint8_t[color_params.BytesPerPixel()]());
    std::unique_ptr<uint8_t[]> transformed_pixel_media(
        new uint8_t[color_params.BytesPerPixel()]());

    SkColorSpaceXform::ColorFormat transformed_color_format =
        color_params.BytesPerPixel() == 4
            ? SkColorSpaceXform::ColorFormat::kRGBA_8888_ColorFormat
            : SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;

    EXPECT_TRUE(color_space_xform_canvas->apply(
        transformed_color_format, transformed_pixel_canvas.get(),
        SkColorSpaceXform::ColorFormat::kRGBA_8888_ColorFormat, src_pixel.get(),
        1, SkAlphaType::kPremul_SkAlphaType));

    EXPECT_TRUE(color_space_xform_media->apply(
        transformed_color_format, transformed_pixel_media.get(),
        SkColorSpaceXform::ColorFormat::kRGBA_8888_ColorFormat, src_pixel.get(),
        1, SkAlphaType::kPremul_SkAlphaType));

    ColorCorrectionTestUtils::CompareColorCorrectedPixels(
        transformed_pixel_canvas.get(), transformed_pixel_media.get(), 1,
        kUint16ArrayStorageFormat, kAlphaMultiplied,
        kUnpremulRoundTripTolerance);
    }
}

}  // namespace blink

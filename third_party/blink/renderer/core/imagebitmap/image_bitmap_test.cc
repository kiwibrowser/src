/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/imagebitmap/image_bitmap.h"

#include "SkPixelRef.h"  // FIXME: qualify this skia header file.

#include "build/build_config.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/core/html/canvas/image_data.h"
#include "third_party/blink/renderer/core/html/html_image_element.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/core/loader/resource/image_resource_content.h"
#include "third_party/blink/renderer/platform/graphics/accelerated_static_bitmap_image.h"
#include "third_party/blink/renderer/platform/graphics/color_correction_test_utils.h"
#include "third_party/blink/renderer/platform/graphics/gpu/shared_gpu_context.h"
#include "third_party/blink/renderer/platform/graphics/skia/skia_utils.h"
#include "third_party/blink/renderer/platform/graphics/static_bitmap_image.h"
#include "third_party/blink/renderer/platform/graphics/test/fake_gles2_interface.h"
#include "third_party/blink/renderer/platform/graphics/test/fake_web_graphics_context_3d_provider.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/image-decoders/image_decoder.h"
#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorSpaceXform.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkSwizzle.h"

namespace blink {

class ExceptionState;

class ImageBitmapTest : public testing::Test {
 protected:
  void SetUp() override {
    sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(10, 10);
    surface->getCanvas()->clear(0xFFFFFFFF);
    image_ = surface->makeImageSnapshot();

    sk_sp<SkSurface> surface2 = SkSurface::MakeRasterN32Premul(5, 5);
    surface2->getCanvas()->clear(0xAAAAAAAA);
    image2_ = surface2->makeImageSnapshot();

    // Save the global memory cache to restore it upon teardown.
    global_memory_cache_ = ReplaceMemoryCacheForTesting(MemoryCache::Create());

    auto factory = [](FakeGLES2Interface* gl, bool* gpu_compositing_disabled)
        -> std::unique_ptr<WebGraphicsContext3DProvider> {
      *gpu_compositing_disabled = false;
      return std::make_unique<FakeWebGraphicsContext3DProvider>(gl, nullptr);
    };
    SharedGpuContext::SetContextProviderFactoryForTesting(
        WTF::BindRepeating(factory, WTF::Unretained(&gl_)));
  }
  void TearDown() override {
    // Garbage collection is required prior to switching out the
    // test's memory cache; image resources are released, evicting
    // them from the cache.
    ThreadState::Current()->CollectGarbage(
        BlinkGC::kNoHeapPointersOnStack, BlinkGC::kAtomicMarking,
        BlinkGC::kEagerSweeping, BlinkGC::kForcedGC);

    ReplaceMemoryCacheForTesting(global_memory_cache_.Release());
    SharedGpuContext::ResetForTesting();
  }

 protected:
  FakeGLES2Interface gl_;
  sk_sp<SkImage> image_, image2_;
  Persistent<MemoryCache> global_memory_cache_;
};

TEST_F(ImageBitmapTest, ImageResourceConsistency) {
  const ImageBitmapOptions default_options;
  HTMLImageElement* image_element =
      HTMLImageElement::Create(*Document::CreateForTest());
  sk_sp<SkColorSpace> src_rgb_color_space = SkColorSpace::MakeSRGB();
  SkImageInfo raster_image_info =
      SkImageInfo::MakeN32Premul(5, 5, src_rgb_color_space);
  sk_sp<SkSurface> surface(SkSurface::MakeRaster(raster_image_info));
  sk_sp<SkImage> image = surface->makeImageSnapshot();
  ImageResourceContent* original_image_resource =
      ImageResourceContent::CreateLoaded(
          StaticBitmapImage::Create(image).get());
  image_element->SetImageForTest(original_image_resource);

  base::Optional<IntRect> crop_rect =
      IntRect(0, 0, image_->width(), image_->height());
  ImageBitmap* image_bitmap_no_crop =
      ImageBitmap::Create(image_element, crop_rect,
                          &(image_element->GetDocument()), default_options);
  ASSERT_TRUE(image_bitmap_no_crop);
  crop_rect = IntRect(image_->width() / 2, image_->height() / 2,
                      image_->width() / 2, image_->height() / 2);
  ImageBitmap* image_bitmap_interior_crop =
      ImageBitmap::Create(image_element, crop_rect,
                          &(image_element->GetDocument()), default_options);
  ASSERT_TRUE(image_bitmap_interior_crop);
  crop_rect = IntRect(-image_->width() / 2, -image_->height() / 2,
                      image_->width(), image_->height());
  ImageBitmap* image_bitmap_exterior_crop =
      ImageBitmap::Create(image_element, crop_rect,
                          &(image_element->GetDocument()), default_options);
  ASSERT_TRUE(image_bitmap_exterior_crop);
  crop_rect = IntRect(-image_->width(), -image_->height(), image_->width(),
                      image_->height());
  ImageBitmap* image_bitmap_outside_crop =
      ImageBitmap::Create(image_element, crop_rect,
                          &(image_element->GetDocument()), default_options);
  ASSERT_TRUE(image_bitmap_outside_crop);

  ASSERT_EQ(image_bitmap_no_crop->BitmapImage()
                ->PaintImageForCurrentFrame()
                .GetSkImage(),
            image_element->CachedImage()
                ->GetImage()
                ->PaintImageForCurrentFrame()
                .GetSkImage());
  ASSERT_NE(image_bitmap_interior_crop->BitmapImage()
                ->PaintImageForCurrentFrame()
                .GetSkImage(),
            image_element->CachedImage()
                ->GetImage()
                ->PaintImageForCurrentFrame()
                .GetSkImage());
  ASSERT_EQ(image_bitmap_exterior_crop->BitmapImage()
                ->PaintImageForCurrentFrame()
                .GetSkImage(),
            image_element->CachedImage()
                ->GetImage()
                ->PaintImageForCurrentFrame()
                .GetSkImage());

  scoped_refptr<StaticBitmapImage> empty_image =
      image_bitmap_outside_crop->BitmapImage();
  ASSERT_NE(empty_image->PaintImageForCurrentFrame().GetSkImage(),
            image_element->CachedImage()
                ->GetImage()
                ->PaintImageForCurrentFrame()
                .GetSkImage());
}

// Verifies that ImageBitmaps constructed from HTMLImageElements hold a
// reference to the original Image if the HTMLImageElement src is changed.
TEST_F(ImageBitmapTest, ImageBitmapSourceChanged) {
  HTMLImageElement* image =
      HTMLImageElement::Create(*Document::CreateForTest());
  sk_sp<SkColorSpace> src_rgb_color_space = SkColorSpace::MakeSRGB();
  SkImageInfo raster_image_info =
      SkImageInfo::MakeN32Premul(5, 5, src_rgb_color_space);
  sk_sp<SkSurface> raster_surface(SkSurface::MakeRaster(raster_image_info));
  sk_sp<SkImage> raster_image = raster_surface->makeImageSnapshot();
  ImageResourceContent* original_image_resource =
      ImageResourceContent::CreateLoaded(
          StaticBitmapImage::Create(raster_image).get());
  image->SetImageForTest(original_image_resource);

  const ImageBitmapOptions default_options;
  base::Optional<IntRect> crop_rect =
      IntRect(0, 0, image_->width(), image_->height());
  ImageBitmap* image_bitmap = ImageBitmap::Create(
      image, crop_rect, &(image->GetDocument()), default_options);
  ASSERT_TRUE(image_bitmap);
  ASSERT_EQ(
      image_bitmap->BitmapImage()->PaintImageForCurrentFrame().GetSkImage(),
      original_image_resource->GetImage()
          ->PaintImageForCurrentFrame()
          .GetSkImage());

  ImageResourceContent* new_image_resource = ImageResourceContent::CreateLoaded(
      StaticBitmapImage::Create(image2_).get());
  image->SetImageForTest(new_image_resource);

  {
    ASSERT_EQ(
        image_bitmap->BitmapImage()->PaintImageForCurrentFrame().GetSkImage(),
        original_image_resource->GetImage()
            ->PaintImageForCurrentFrame()
            .GetSkImage());
    SkImage* image1 = image_bitmap->BitmapImage()
                          ->PaintImageForCurrentFrame()
                          .GetSkImage()
                          .get();
    ASSERT_NE(image1, nullptr);
    SkImage* image2 = original_image_resource->GetImage()
                          ->PaintImageForCurrentFrame()
                          .GetSkImage()
                          .get();
    ASSERT_NE(image2, nullptr);
    ASSERT_EQ(image1, image2);
  }

  {
    ASSERT_NE(
        image_bitmap->BitmapImage()->PaintImageForCurrentFrame().GetSkImage(),
        new_image_resource->GetImage()
            ->PaintImageForCurrentFrame()
            .GetSkImage());
    SkImage* image1 = image_bitmap->BitmapImage()
                          ->PaintImageForCurrentFrame()
                          .GetSkImage()
                          .get();
    ASSERT_NE(image1, nullptr);
    SkImage* image2 = new_image_resource->GetImage()
                          ->PaintImageForCurrentFrame()
                          .GetSkImage()
                          .get();
    ASSERT_NE(image2, nullptr);
    ASSERT_NE(image1, image2);
  }
}

static void TestImageBitmapTextureBacked(
    scoped_refptr<StaticBitmapImage> bitmap,
    IntRect& rect,
    ImageBitmapOptions options,
    bool is_texture_backed) {
  ImageBitmap* image_bitmap = ImageBitmap::Create(bitmap, rect, options);
  EXPECT_TRUE(image_bitmap);
  EXPECT_EQ(image_bitmap->BitmapImage()->IsTextureBacked(), is_texture_backed);
}

TEST_F(ImageBitmapTest, AvoidGPUReadback) {
  base::WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper =
      SharedGpuContext::ContextProviderWrapper();
  GrContext* gr = context_provider_wrapper->ContextProvider()->GetGrContext();
  SkImageInfo imageInfo = SkImageInfo::MakeN32Premul(100, 100);

  sk_sp<SkSurface> surface =
      SkSurface::MakeRenderTarget(gr, SkBudgeted::kNo, imageInfo);
  sk_sp<SkImage> image = surface->makeImageSnapshot();

  scoped_refptr<AcceleratedStaticBitmapImage> bitmap =
      AcceleratedStaticBitmapImage::CreateFromSkImage(image,
                                                      context_provider_wrapper);
  EXPECT_TRUE(bitmap->TextureHolderForTesting()->IsSkiaTextureHolder());

  ImageBitmap* image_bitmap = ImageBitmap::Create(bitmap);
  EXPECT_TRUE(image_bitmap);
  EXPECT_TRUE(image_bitmap->BitmapImage()->IsTextureBacked());

  IntRect image_bitmap_rect(25, 25, 50, 50);
  ImageBitmapOptions image_bitmap_options;
  TestImageBitmapTextureBacked(bitmap, image_bitmap_rect, image_bitmap_options,
                               true);

  std::list<String> image_orientations = {"none", "flipY"};
  std::list<String> premultiply_alphas = {"none", "premultiply", "default"};
  std::list<String> color_space_conversions = {"none",       "default", "srgb",
                                               "linear-rgb", "rec2020", "p3"};
  std::list<int> resize_widths = {25, 50, 75};
  std::list<int> resize_heights = {25, 50, 75};
  std::list<String> resize_qualities = {"pixelated", "low", "medium", "high"};

  for (auto image_orientation : image_orientations) {
    for (auto premultiply_alpha : premultiply_alphas) {
      for (auto color_space_conversion : color_space_conversions) {
        for (auto resize_width : resize_widths) {
          for (auto resize_height : resize_heights) {
            for (auto resize_quality : resize_qualities) {
              ImageBitmapOptions image_bitmap_options;
              image_bitmap_options.setImageOrientation(image_orientation);
              image_bitmap_options.setPremultiplyAlpha(premultiply_alpha);
              image_bitmap_options.setColorSpaceConversion(
                  color_space_conversion);
              image_bitmap_options.setResizeWidth(resize_width);
              image_bitmap_options.setResizeHeight(resize_height);
              image_bitmap_options.setResizeQuality(resize_quality);
              // Setting premuliply_alpha to none will cause a read back.
              // Otherwise, we expect to avoid GPU readback when creaing an
              // ImageBitmap from a texture-backed source.
              TestImageBitmapTextureBacked(bitmap, image_bitmap_rect,
                                           image_bitmap_options,
                                           premultiply_alpha != "none");
            }
          }
        }
      }
    }
  }
}

enum class ColorSpaceConversion : uint8_t {
  NONE = 0,
  DEFAULT_COLOR_CORRECTED = 1,
  SRGB = 2,
  LINEAR_RGB = 3,
  P3 = 4,
  REC2020 = 5,

  LAST = REC2020
};

static ImageBitmapOptions PrepareBitmapOptions(
    const ColorSpaceConversion& color_space_conversion) {
  // Set the color space conversion in ImageBitmapOptions
  ImageBitmapOptions options;
  static const Vector<String> kConversions = {
      "none", "default", "srgb", "linear-rgb", "p3", "rec2020"};
  options.setColorSpaceConversion(
      kConversions[static_cast<uint8_t>(color_space_conversion)]);
  return options;
}

TEST_F(ImageBitmapTest, ImageBitmapColorSpaceConversionHTMLImageElement) {
  HTMLImageElement* image_element =
      HTMLImageElement::Create(*Document::CreateForTest());

  SkPaint p;
  p.setColor(SK_ColorRED);
  sk_sp<SkColorSpace> src_rgb_color_space = SkColorSpace::MakeSRGB();

  SkImageInfo raster_image_info =
      SkImageInfo::MakeN32Premul(10, 10, src_rgb_color_space);
  sk_sp<SkSurface> surface(SkSurface::MakeRaster(raster_image_info));
  surface->getCanvas()->drawCircle(5, 5, 5, p);
  sk_sp<SkImage> image = surface->makeImageSnapshot();

  std::unique_ptr<uint8_t[]> src_pixel(
      new uint8_t[raster_image_info.bytesPerPixel()]());
  SkImageInfo src_pixel_image_info = raster_image_info.makeWH(1, 1);
  EXPECT_TRUE(image->readPixels(src_pixel_image_info, src_pixel.get(),
                                src_pixel_image_info.minRowBytes(), 5, 5));

  ImageResourceContent* original_image_resource =
      ImageResourceContent::CreateLoaded(
          StaticBitmapImage::Create(image).get());
  image_element->SetImageForTest(original_image_resource);

  base::Optional<IntRect> crop_rect =
      IntRect(0, 0, image->width(), image->height());

  // Create and test the ImageBitmap objects.
  // We don't check "none" color space conversion as it requires the encoded
  // data in a format readable by ImageDecoder. Furthermore, the code path for
  // "none" color space conversion is not affected by this CL.

  sk_sp<SkColorSpace> color_space = nullptr;
  SkColorType color_type = SkColorType::kN32_SkColorType;
  SkColorSpaceXform::ColorFormat color_format32 =
      (color_type == kBGRA_8888_SkColorType)
          ? SkColorSpaceXform::ColorFormat::kBGRA_8888_ColorFormat
          : SkColorSpaceXform::ColorFormat::kRGBA_8888_ColorFormat;
  SkColorSpaceXform::ColorFormat color_format = color_format32;

  for (uint8_t i =
           static_cast<uint8_t>(ColorSpaceConversion::DEFAULT_COLOR_CORRECTED);
       i <= static_cast<uint8_t>(ColorSpaceConversion::LAST); i++) {
    ColorSpaceConversion color_space_conversion =
        static_cast<ColorSpaceConversion>(i);
    ImageBitmapOptions options =
        PrepareBitmapOptions(color_space_conversion);
    ImageBitmap* image_bitmap = ImageBitmap::Create(
        image_element, crop_rect, &(image_element->GetDocument()), options);
    ASSERT_TRUE(image_bitmap);
    SkImage* converted_image = image_bitmap->BitmapImage()
                                   ->PaintImageForCurrentFrame()
                                   .GetSkImage()
                                   .get();

    switch (color_space_conversion) {
      case ColorSpaceConversion::NONE:
        NOTREACHED();
        break;
      case ColorSpaceConversion::DEFAULT_COLOR_CORRECTED:
      case ColorSpaceConversion::SRGB:
        color_space = SkColorSpace::MakeSRGB();
        color_format = color_format32;
        break;
      case ColorSpaceConversion::LINEAR_RGB:
        color_space = SkColorSpace::MakeSRGBLinear();
        color_type = SkColorType::kRGBA_F16_SkColorType;
        color_format = SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;
        break;
      case ColorSpaceConversion::P3:
        color_space =
            SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                  SkColorSpace::kDCIP3_D65_Gamut);
        color_type = SkColorType::kRGBA_F16_SkColorType;
        color_format = SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;
        break;
      case ColorSpaceConversion::REC2020:
        color_space =
            SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                  SkColorSpace::kRec2020_Gamut);
        color_type = SkColorType::kRGBA_F16_SkColorType;
        color_format = SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;
        break;
      default:
        NOTREACHED();
    }

    SkImageInfo image_info = SkImageInfo::Make(
        1, 1, color_type, SkAlphaType::kPremul_SkAlphaType, color_space);
    std::unique_ptr<uint8_t[]> converted_pixel(
        new uint8_t[image_info.bytesPerPixel()]());
    EXPECT_TRUE(converted_image->readPixels(image_info, converted_pixel.get(),
                                            image_info.minRowBytes(), 5, 5));

    // Transform the source pixel and check if the image bitmap color conversion
    // is done correctly.
    std::unique_ptr<SkColorSpaceXform> color_space_xform =
        SkColorSpaceXform::New(src_rgb_color_space.get(), color_space.get());
    std::unique_ptr<uint8_t[]> transformed_pixel(
        new uint8_t[image_info.bytesPerPixel()]());
    EXPECT_TRUE(color_space_xform->apply(color_format, transformed_pixel.get(),
                                         color_format32, src_pixel.get(), 1,
                                         SkAlphaType::kPremul_SkAlphaType));

    ColorCorrectionTestUtils::CompareColorCorrectedPixels(
        converted_pixel.get(), transformed_pixel.get(), 1,
        (color_type == kN32_SkColorType) ? kUint8ClampedArrayStorageFormat
                                         : kUint16ArrayStorageFormat,
        kAlphaMultiplied, kUnpremulRoundTripTolerance);
  }
}

TEST_F(ImageBitmapTest, ImageBitmapColorSpaceConversionImageBitmap) {
  HTMLImageElement* image_element =
      HTMLImageElement::Create(*Document::CreateForTest());

  SkPaint p;
  p.setColor(SK_ColorRED);
  sk_sp<SkColorSpace> src_rgb_color_space = SkColorSpace::MakeSRGB();

  SkImageInfo raster_image_info =
      SkImageInfo::MakeN32Premul(10, 10, src_rgb_color_space);
  sk_sp<SkSurface> surface(SkSurface::MakeRaster(raster_image_info));
  surface->getCanvas()->drawCircle(5, 5, 5, p);
  sk_sp<SkImage> image = surface->makeImageSnapshot();

  std::unique_ptr<uint8_t[]> src_pixel(
      new uint8_t[raster_image_info.bytesPerPixel()]());
  EXPECT_TRUE(image->readPixels(raster_image_info.makeWH(1, 1), src_pixel.get(),
                                raster_image_info.minRowBytes(), 5, 5));

  ImageResourceContent* source_image_resource =
      ImageResourceContent::CreateLoaded(
          StaticBitmapImage::Create(image).get());
  image_element->SetImageForTest(source_image_resource);

  base::Optional<IntRect> crop_rect =
      IntRect(0, 0, image->width(), image->height());
  ImageBitmapOptions options =
      PrepareBitmapOptions(ColorSpaceConversion::SRGB);
  ImageBitmap* source_image_bitmap = ImageBitmap::Create(
      image_element, crop_rect, &(image_element->GetDocument()), options);
  ASSERT_TRUE(source_image_bitmap);

  sk_sp<SkColorSpace> color_space = nullptr;
  SkColorType color_type = SkColorType::kN32_SkColorType;
  SkColorSpaceXform::ColorFormat color_format32 =
      (color_type == kBGRA_8888_SkColorType)
          ? SkColorSpaceXform::ColorFormat::kBGRA_8888_ColorFormat
          : SkColorSpaceXform::ColorFormat::kRGBA_8888_ColorFormat;
  SkColorSpaceXform::ColorFormat color_format = color_format32;

  for (uint8_t i =
           static_cast<uint8_t>(ColorSpaceConversion::DEFAULT_COLOR_CORRECTED);
       i <= static_cast<uint8_t>(ColorSpaceConversion::LAST); i++) {
    ColorSpaceConversion color_space_conversion =
        static_cast<ColorSpaceConversion>(i);
    options = PrepareBitmapOptions(color_space_conversion);
    ImageBitmap* image_bitmap =
        ImageBitmap::Create(source_image_bitmap, crop_rect, options);
    ASSERT_TRUE(image_bitmap);
    SkImage* converted_image = image_bitmap->BitmapImage()
                                   ->PaintImageForCurrentFrame()
                                   .GetSkImage()
                                   .get();

    switch (color_space_conversion) {
      case ColorSpaceConversion::NONE:
        NOTREACHED();
        break;
      case ColorSpaceConversion::DEFAULT_COLOR_CORRECTED:
      case ColorSpaceConversion::SRGB:
        color_space = SkColorSpace::MakeSRGB();
        color_format = color_format32;
        break;
      case ColorSpaceConversion::LINEAR_RGB:
        color_space = SkColorSpace::MakeSRGBLinear();
        color_type = SkColorType::kRGBA_F16_SkColorType;
        color_format = SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;
        break;
      case ColorSpaceConversion::P3:
        color_space =
            SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                  SkColorSpace::kDCIP3_D65_Gamut);
        color_type = SkColorType::kRGBA_F16_SkColorType;
        color_format = SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;
        break;
      case ColorSpaceConversion::REC2020:
        color_space =
            SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                  SkColorSpace::kRec2020_Gamut);
        color_type = SkColorType::kRGBA_F16_SkColorType;
        color_format = SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;
        break;
      default:
        NOTREACHED();
    }

    SkImageInfo image_info = SkImageInfo::Make(
        1, 1, color_type, SkAlphaType::kPremul_SkAlphaType, color_space);
    std::unique_ptr<uint8_t[]> converted_pixel(
        new uint8_t[image_info.bytesPerPixel()]());
    converted_image->readPixels(
        image_info, converted_pixel.get(),
        converted_image->width() * image_info.bytesPerPixel(), 5, 5);

    // Transform the source pixel and check if the image bitmap color conversion
    // is done correctly.
    std::unique_ptr<SkColorSpaceXform> color_space_xform =
        SkColorSpaceXform::New(src_rgb_color_space.get(), color_space.get());
    std::unique_ptr<uint8_t[]> transformed_pixel(
        new uint8_t[image_info.bytesPerPixel()]());
    EXPECT_TRUE(color_space_xform->apply(color_format, transformed_pixel.get(),
                                         color_format32, src_pixel.get(), 1,
                                         SkAlphaType::kPremul_SkAlphaType));

    ColorCorrectionTestUtils::CompareColorCorrectedPixels(
        converted_pixel.get(), transformed_pixel.get(), 1,
        (color_type == kN32_SkColorType) ? kUint8ClampedArrayStorageFormat
                                         : kUint16ArrayStorageFormat,
        kAlphaMultiplied, kUnpremulRoundTripTolerance);
  }
}

TEST_F(ImageBitmapTest, ImageBitmapColorSpaceConversionStaticBitmapImage) {
  SkPaint p;
  p.setColor(SK_ColorRED);
  sk_sp<SkColorSpace> src_rgb_color_space = SkColorSpace::MakeSRGB();

  SkImageInfo raster_image_info =
      SkImageInfo::MakeN32Premul(10, 10, src_rgb_color_space);
  sk_sp<SkSurface> surface(SkSurface::MakeRaster(raster_image_info));
  surface->getCanvas()->drawCircle(5, 5, 5, p);
  sk_sp<SkImage> image = surface->makeImageSnapshot();

  std::unique_ptr<uint8_t[]> src_pixel(
      new uint8_t[raster_image_info.bytesPerPixel()]());
  image->readPixels(raster_image_info.makeWH(1, 1), src_pixel.get(),
                    image->width() * raster_image_info.bytesPerPixel(), 5, 5);

  base::Optional<IntRect> crop_rect =
      IntRect(0, 0, image->width(), image->height());

  sk_sp<SkColorSpace> color_space = nullptr;
  SkColorType color_type = SkColorType::kN32_SkColorType;
  SkColorSpaceXform::ColorFormat color_format32 =
      (color_type == kBGRA_8888_SkColorType)
          ? SkColorSpaceXform::ColorFormat::kBGRA_8888_ColorFormat
          : SkColorSpaceXform::ColorFormat::kRGBA_8888_ColorFormat;
  SkColorSpaceXform::ColorFormat color_format = color_format32;

  for (uint8_t i =
           static_cast<uint8_t>(ColorSpaceConversion::DEFAULT_COLOR_CORRECTED);
       i <= static_cast<uint8_t>(ColorSpaceConversion::LAST); i++) {
    ColorSpaceConversion color_space_conversion =
        static_cast<ColorSpaceConversion>(i);
    ImageBitmapOptions options =
        PrepareBitmapOptions(color_space_conversion);
    ImageBitmap* image_bitmap = ImageBitmap::Create(
        StaticBitmapImage::Create(image), crop_rect, options);
    ASSERT_TRUE(image_bitmap);

    SkImage* converted_image = image_bitmap->BitmapImage()
                                   ->PaintImageForCurrentFrame()
                                   .GetSkImage()
                                   .get();

    UnpremulRoundTripTolerance unpremul_round_trip_tolerance =
        kUnpremulRoundTripTolerance;
    switch (color_space_conversion) {
      case ColorSpaceConversion::NONE:
        NOTREACHED();
        break;
      case ColorSpaceConversion::DEFAULT_COLOR_CORRECTED:
      case ColorSpaceConversion::SRGB:
        color_space = SkColorSpace::MakeSRGB();
        color_format = color_format32;
        unpremul_round_trip_tolerance = kUnpremulRoundTripTolerance;
        break;
      case ColorSpaceConversion::LINEAR_RGB:
        color_space = SkColorSpace::MakeSRGBLinear();
        color_type = SkColorType::kRGBA_F16_SkColorType;
        color_format = SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;
        break;
      case ColorSpaceConversion::P3:
        color_space =
            SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                  SkColorSpace::kDCIP3_D65_Gamut);
        color_type = SkColorType::kRGBA_F16_SkColorType;
        color_format = SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;
        break;
      case ColorSpaceConversion::REC2020:
        color_space =
            SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                  SkColorSpace::kRec2020_Gamut);
        color_type = SkColorType::kRGBA_F16_SkColorType;
        color_format = SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;
        break;
      default:
        NOTREACHED();
    }

    SkImageInfo image_info = SkImageInfo::Make(
        1, 1, color_type, SkAlphaType::kPremul_SkAlphaType, color_space);
    std::unique_ptr<uint8_t[]> converted_pixel(
        new uint8_t[image_info.bytesPerPixel()]());
    converted_image->readPixels(
        image_info, converted_pixel.get(),
        converted_image->width() * image_info.bytesPerPixel(), 5, 5);

    // Transform the source pixel and check if the image bitmap color conversion
    // is done correctly.
    std::unique_ptr<SkColorSpaceXform> color_space_xform =
        SkColorSpaceXform::New(src_rgb_color_space.get(), color_space.get());
    std::unique_ptr<uint8_t[]> transformed_pixel(
        new uint8_t[image_info.bytesPerPixel()]());
    EXPECT_TRUE(color_space_xform->apply(color_format, transformed_pixel.get(),
                                         color_format32, src_pixel.get(), 1,
                                         SkAlphaType::kPremul_SkAlphaType));

    ColorCorrectionTestUtils::CompareColorCorrectedPixels(
        converted_pixel.get(), transformed_pixel.get(), 1,
        (color_type == kN32_SkColorType) ? kUint8ClampedArrayStorageFormat
                                         : kUint16ArrayStorageFormat,
        kAlphaMultiplied, unpremul_round_trip_tolerance);
  }
}

TEST_F(ImageBitmapTest, ImageBitmapColorSpaceConversionImageData) {
  sk_sp<SkColorSpace> src_rgb_color_space = SkColorSpace::MakeSRGB();
  unsigned char data_buffer[4] = {32, 96, 160, 128};
  DOMUint8ClampedArray* data = DOMUint8ClampedArray::Create(data_buffer, 4);
  ImageDataColorSettings color_settings;
  ImageData* image_data = ImageData::Create(
      IntSize(1, 1), NotShared<DOMUint8ClampedArray>(data), &color_settings);
  std::unique_ptr<uint8_t[]> src_pixel(new uint8_t[4]());
  memcpy(src_pixel.get(), image_data->data()->Data(), 4);

  base::Optional<IntRect> crop_rect = IntRect(0, 0, 1, 1);
  sk_sp<SkColorSpace> color_space = nullptr;
  SkColorType color_type = SkColorType::kN32_SkColorType;
  SkColorSpaceXform::ColorFormat color_format32 =
      (color_type == kBGRA_8888_SkColorType)
          ? SkColorSpaceXform::ColorFormat::kBGRA_8888_ColorFormat
          : SkColorSpaceXform::ColorFormat::kRGBA_8888_ColorFormat;
  SkColorSpaceXform::ColorFormat color_format = color_format32;

  for (uint8_t i =
           static_cast<uint8_t>(ColorSpaceConversion::DEFAULT_COLOR_CORRECTED);
       i <= static_cast<uint8_t>(ColorSpaceConversion::LAST); i++) {
    ColorSpaceConversion color_space_conversion =
        static_cast<ColorSpaceConversion>(i);
    ImageBitmapOptions options =
        PrepareBitmapOptions(color_space_conversion);
    ImageBitmap* image_bitmap =
        ImageBitmap::Create(image_data, crop_rect, options);
    ASSERT_TRUE(image_bitmap);

    SkImage* converted_image = image_bitmap->BitmapImage()
                                   ->PaintImageForCurrentFrame()
                                   .GetSkImage()
                                   .get();

    switch (color_space_conversion) {
      case ColorSpaceConversion::NONE:
        NOTREACHED();
        break;
      case ColorSpaceConversion::DEFAULT_COLOR_CORRECTED:
      case ColorSpaceConversion::SRGB:
        color_space = SkColorSpace::MakeSRGB();
        color_format = color_format32;
        break;
      case ColorSpaceConversion::LINEAR_RGB:
        color_space = SkColorSpace::MakeSRGBLinear();
        color_type = SkColorType::kRGBA_F16_SkColorType;
        color_format = SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;
        break;
      case ColorSpaceConversion::P3:
        color_space =
            SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                  SkColorSpace::kDCIP3_D65_Gamut);
        color_type = SkColorType::kRGBA_F16_SkColorType;
        color_format = SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;
        break;
      case ColorSpaceConversion::REC2020:
        color_space =
            SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                  SkColorSpace::kRec2020_Gamut);
        color_type = SkColorType::kRGBA_F16_SkColorType;
        color_format = SkColorSpaceXform::ColorFormat::kRGBA_F16_ColorFormat;
        break;
      default:
        NOTREACHED();
    }

    SkImageInfo image_info =
        SkImageInfo::Make(1, 1, color_type, SkAlphaType::kUnpremul_SkAlphaType);
    std::unique_ptr<uint8_t[]> converted_pixel(
        new uint8_t[image_info.bytesPerPixel()]());
    converted_image->readPixels(
        image_info, converted_pixel.get(),
        converted_image->width() * image_info.bytesPerPixel(), 0, 0);

    // Transform the source pixel and check if the pixel from image bitmap has
    // the same color information.
    std::unique_ptr<SkColorSpaceXform> color_space_xform =
        SkColorSpaceXform::New(src_rgb_color_space.get(), color_space.get());
    std::unique_ptr<uint8_t[]> transformed_pixel(
        new uint8_t[image_info.bytesPerPixel()]());
    EXPECT_TRUE(color_space_xform->apply(
        color_format, transformed_pixel.get(),
        SkColorSpaceXform::ColorFormat::kRGBA_8888_ColorFormat, src_pixel.get(),
        1, kUnpremul_SkAlphaType));

    ColorCorrectionTestUtils::CompareColorCorrectedPixels(
        converted_pixel.get(), transformed_pixel.get(), 1,
        (color_type == kN32_SkColorType) ? kUint8ClampedArrayStorageFormat
                                         : kUint16ArrayStorageFormat,
        kAlphaUnmultiplied, kUnpremulRoundTripTolerance);
  }
}

// This test is failing on asan-clang-phone because memory allocation is
// declined. See <http://crbug.com/782286>.
#if defined(OS_ANDROID)
#define MAYBE_CreateImageBitmapFromTooBigImageDataDoesNotCrash \
  DISABLED_CreateImageBitmapFromTooBigImageDataDoesNotCrash
#else
#define MAYBE_CreateImageBitmapFromTooBigImageDataDoesNotCrash \
  CreateImageBitmapFromTooBigImageDataDoesNotCrash
#endif

// This test verifies if requesting a large ImageData and creating an
// ImageBitmap from that does not crash. crbug.com/780358
TEST_F(ImageBitmapTest,
       MAYBE_CreateImageBitmapFromTooBigImageDataDoesNotCrash) {
  ImageData* image_data =
      ImageData::CreateForTest(IntSize(v8::TypedArray::kMaxLength / 16, 1));
  DCHECK(image_data);
  ImageBitmapOptions options =
      PrepareBitmapOptions(ColorSpaceConversion::DEFAULT_COLOR_CORRECTED);
  ImageBitmap* image_bitmap = ImageBitmap::Create(
      image_data, IntRect(IntPoint(0, 0), image_data->Size()), options);
  DCHECK(image_bitmap);
}

#undef MAYBE_ImageBitmapColorSpaceConversionHTMLImageElement
}  // namespace blink

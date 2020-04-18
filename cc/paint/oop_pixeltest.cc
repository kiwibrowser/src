// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tuple>
#include <vector>

#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/base/region.h"
#include "cc/layers/recording_source.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/paint_filter.h"
#include "cc/paint/paint_image_builder.h"
#include "cc/paint/paint_text_blob_builder.h"
#include "cc/raster/playback_image_provider.h"
#include "cc/raster/raster_source.h"
#include "cc/test/pixel_test_utils.h"
#include "cc/test/test_in_process_context_provider.h"
#include "cc/tiles/gpu_image_decode_cache.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/raster_implementation_gles.h"
#include "gpu/command_buffer/client/shared_memory_limits.h"
#include "gpu/command_buffer/common/context_creation_attribs.h"
#include "gpu/config/gpu_switches.h"
#include "gpu/ipc/gl_in_process_context.h"
#include "gpu/skia_bindings/grcontext_for_gles2_interface.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/include/gpu/GrTypes.h"
#include "ui/gfx/geometry/axis_transform2d.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/skia_util.h"
#include "ui/gl/gl_implementation.h"

namespace cc {
namespace {

scoped_refptr<DisplayItemList> MakeNoopDisplayItemList() {
  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  display_item_list->push<SaveOp>();
  display_item_list->push<RestoreOp>();
  display_item_list->EndPaintOfUnpaired(gfx::Rect(10000, 10000));
  display_item_list->Finalize();
  return display_item_list;
}

class OopPixelTest : public testing::Test {
 public:
  void SetUp() override {
    raster_context_provider_ =
        base::MakeRefCounted<TestInProcessContextProvider>(
            /*enable_oop_rasterization=*/true);
    const int raster_max_texture_size =
        raster_context_provider_->ContextCapabilities().max_texture_size;
    oop_image_cache_.reset(new GpuImageDecodeCache(
        raster_context_provider_.get(), true, kRGBA_8888_SkColorType,
        kWorkingSetSize, raster_max_texture_size));

    gles2_context_provider_ =
        base::MakeRefCounted<TestInProcessContextProvider>(
            /*enable_oop_rasterization=*/false);
    const int gles2_max_texture_size =
        raster_context_provider_->ContextCapabilities().max_texture_size;
    gpu_image_cache_.reset(new GpuImageDecodeCache(
        gles2_context_provider_.get(), false, kRGBA_8888_SkColorType,
        kWorkingSetSize, gles2_max_texture_size));

    ASSERT_EQ(raster_max_texture_size, gles2_max_texture_size);
  }

  class RasterOptions {
   public:
    RasterOptions() = default;
    explicit RasterOptions(const gfx::Size& playback_size) {
      resource_size = playback_size;
      content_size = resource_size;
      full_raster_rect = gfx::Rect(playback_size);
      playback_rect = gfx::Rect(playback_size);
    }

    SkColor background_color = SK_ColorBLACK;
    int msaa_sample_count = 0;
    bool use_lcd_text = false;
    SkColorType color_type = kRGBA_8888_SkColorType;
    gfx::Size resource_size;
    gfx::Size content_size;
    gfx::Rect full_raster_rect;
    gfx::Rect playback_rect;
    gfx::Vector2dF post_translate = {0.f, 0.f};
    float post_scale = 1.f;
    gfx::ColorSpace color_space = gfx::ColorSpace::CreateSRGB();
    bool requires_clear = false;
    bool preclear = false;
    SkColor preclear_color;
    ImageDecodeCache* image_cache = nullptr;
  };

  SkBitmap Raster(scoped_refptr<DisplayItemList> display_item_list,
                  const gfx::Size& playback_size) {
    RasterOptions options(playback_size);
    return Raster(display_item_list, options);
  }

  SkBitmap Raster(scoped_refptr<DisplayItemList> display_item_list,
                  const RasterOptions& options) {
    TestInProcessContextProvider::ScopedRasterContextLock lock(
        raster_context_provider_.get());

    PlaybackImageProvider image_provider(oop_image_cache_.get(),
                                         options.color_space,
                                         PlaybackImageProvider::Settings());

    gpu::gles2::GLES2Interface* gl = gles2_context_provider_->ContextGL();
    int width = options.resource_size.width();
    int height = options.resource_size.height();

    // Create and allocate a texture on the raster interface.
    GLuint raster_texture_id;

    auto* raster_implementation = raster_context_provider_->RasterInterface();
    raster_texture_id = raster_implementation->CreateTexture(
        false, gfx::BufferUsage::GPU_READ, viz::ResourceFormat::RGBA_8888);
    raster_implementation->TexStorage2D(raster_texture_id, 1, width, height);
    raster_implementation->TexParameteri(raster_texture_id,
                                         GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    EXPECT_EQ(raster_implementation->GetError(),
              static_cast<unsigned>(GL_NO_ERROR));

    RasterColorSpace color_space(options.color_space, ++color_space_id_);

    if (options.preclear) {
      raster_implementation->BeginRasterCHROMIUM(
          raster_texture_id, options.preclear_color, options.msaa_sample_count,
          options.use_lcd_text, options.color_type, color_space);
      raster_implementation->EndRasterCHROMIUM();
    }

    // "Out of process" raster! \o/

    raster_implementation->BeginRasterCHROMIUM(
        raster_texture_id, options.background_color, options.msaa_sample_count,
        options.use_lcd_text, options.color_type, color_space);
    raster_implementation->RasterCHROMIUM(
        display_item_list.get(), &image_provider, options.content_size,
        options.full_raster_rect, options.playback_rect, options.post_translate,
        options.post_scale, options.requires_clear);
    raster_implementation->EndRasterCHROMIUM();

    // Produce a mailbox and insert an ordering barrier (assumes the raster
    // interface and gl are on the same scheduling group).
    gpu::Mailbox mailbox;
    raster_implementation->GenMailbox(mailbox.name);
    raster_implementation->ProduceTextureDirect(raster_texture_id,
                                                mailbox.name);
    raster_implementation->OrderingBarrierCHROMIUM();

    EXPECT_EQ(raster_implementation->GetError(),
              static_cast<unsigned>(GL_NO_ERROR));

    // Import the texture in gl, create an fbo and bind the texture to it.
    GLuint gl_texture_id = gl->CreateAndConsumeTextureCHROMIUM(mailbox.name);
    GLuint fbo_id;
    gl->GenFramebuffers(1, &fbo_id);
    gl->BindFramebuffer(GL_FRAMEBUFFER, fbo_id);
    gl->FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, gl_texture_id, 0);

    // Read the data back.
    std::unique_ptr<unsigned char[]> data(
        new unsigned char[width * height * 4]);
    gl->ReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data.get());

    gl->DeleteTextures(1, &gl_texture_id);
    gl->DeleteFramebuffers(1, &fbo_id);

    gl->OrderingBarrierCHROMIUM();
    raster_implementation->DeleteTextures(1, &raster_texture_id);

    // Swizzle rgba->bgra if needed.
    std::vector<SkPMColor> colors;
    colors.reserve(width * height);
    for (int h = 0; h < height; ++h) {
      for (int w = 0; w < width; ++w) {
        int i = (h * width + w) * 4;
        colors.push_back(SkPreMultiplyARGB(data[i + 3], data[i + 0],
                                           data[i + 1], data[i + 2]));
      }
    }

    SkBitmap bitmap;
    bitmap.allocN32Pixels(width, height);
    SkPixmap pixmap(SkImageInfo::MakeN32Premul(options.resource_size.width(),
                                               options.resource_size.height()),
                    colors.data(),
                    options.resource_size.width() * sizeof(SkColor));
    bitmap.writePixels(pixmap);
    return bitmap;
  }

  SkBitmap RasterExpectedBitmap(
      scoped_refptr<DisplayItemList> display_item_list,
      const gfx::Size& playback_size) {
    RasterOptions options(playback_size);
    return RasterExpectedBitmap(display_item_list, options);
  }

  SkBitmap RasterExpectedBitmap(
      scoped_refptr<DisplayItemList> display_item_list,
      const RasterOptions& options) {
    TestInProcessContextProvider::ScopedRasterContextLock lock(
        gles2_context_provider_.get());
    gles2_context_provider_->GrContext()->resetContext();

    // Generate bitmap via the "in process" raster path.  This verifies
    // that the preamble setup in RasterSource::PlaybackToCanvas matches
    // the same setup done in GLES2Implementation::RasterCHROMIUM.
    RecordingSource recording;
    recording.UpdateDisplayItemList(display_item_list, 0u, 1.f);
    recording.SetBackgroundColor(options.background_color);
    Region fake_invalidation;
    gfx::Rect layer_rect(gfx::Size(options.full_raster_rect.right(),
                                   options.full_raster_rect.bottom()));
    recording.UpdateAndExpandInvalidation(&fake_invalidation, layer_rect.size(),
                                          layer_rect);
    recording.SetRequiresClear(options.requires_clear);

    PlaybackImageProvider image_provider(gpu_image_cache_.get(),
                                         options.color_space,
                                         PlaybackImageProvider::Settings());

    auto raster_source = recording.CreateRasterSource();
    RasterSource::PlaybackSettings settings;
    settings.use_lcd_text = options.use_lcd_text;
    settings.image_provider = &image_provider;

    uint32_t flags = 0;
    SkSurfaceProps surface_props(flags, kUnknown_SkPixelGeometry);
    if (options.use_lcd_text) {
      surface_props =
          SkSurfaceProps(flags, SkSurfaceProps::kLegacyFontHost_InitType);
    }
    SkImageInfo image_info = SkImageInfo::MakeN32Premul(
        options.resource_size.width(), options.resource_size.height());
    auto surface = SkSurface::MakeRenderTarget(
        gles2_context_provider_->GrContext(), SkBudgeted::kYes, image_info);
    SkCanvas* canvas = surface->getCanvas();
    if (options.preclear)
      canvas->drawColor(options.preclear_color);
    else
      canvas->drawColor(options.background_color);

    gfx::AxisTransform2d raster_transform(options.post_scale,
                                          options.post_translate);
    raster_source->PlaybackToCanvas(
        canvas, options.color_space, options.content_size,
        options.full_raster_rect, options.playback_rect, raster_transform,
        settings);
    surface->prepareForExternalIO();
    EXPECT_EQ(gles2_context_provider_->ContextGL()->GetError(),
              static_cast<unsigned>(GL_NO_ERROR));

    SkBitmap bitmap;
    SkImageInfo info = SkImageInfo::Make(
        options.resource_size.width(), options.resource_size.height(),
        SkColorType::kBGRA_8888_SkColorType, SkAlphaType::kPremul_SkAlphaType);
    bitmap.allocPixels(info, options.resource_size.width() * 4);
    bool success = surface->readPixels(bitmap, 0, 0);
    CHECK(success);
    EXPECT_EQ(gles2_context_provider_->ContextGL()->GetError(),
              static_cast<unsigned>(GL_NO_ERROR));
    return bitmap;
  }

  void ExpectEquals(SkBitmap actual,
                    SkBitmap expected,
                    const char* label = nullptr) {
    EXPECT_EQ(actual.dimensions(), expected.dimensions());
    auto expected_url = GetPNGDataUrl(expected);
    auto actual_url = GetPNGDataUrl(actual);
    if (actual_url == expected_url)
      return;
    if (label) {
      ADD_FAILURE() << "\nCase: " << label << "\nExpected: " << expected_url
                    << "\nActual:   " << actual_url;
    } else {
      ADD_FAILURE() << "\nExpected: " << expected_url
                    << "\nActual:   " << actual_url;
    }
  }

 protected:
  enum { kWorkingSetSize = 64 * 1024 * 1024 };
  scoped_refptr<TestInProcessContextProvider> raster_context_provider_;
  scoped_refptr<TestInProcessContextProvider> gles2_context_provider_;
  std::unique_ptr<GpuImageDecodeCache> gpu_image_cache_;
  std::unique_ptr<GpuImageDecodeCache> oop_image_cache_;
  gl::DisableNullDrawGLBindings enable_pixel_output_;
  std::unique_ptr<ImageProvider> image_provider_;
  int color_space_id_ = 0;
};

class OopImagePixelTest : public OopPixelTest,
                          public ::testing::WithParamInterface<bool> {
 public:
  bool UseTooLargeImage() { return GetParam(); }
  SkFilterQuality FilterQuality() { return kNone_SkFilterQuality; }

  gfx::Size GetImageSize() {
    const int kMaxSize = 20000;
    DCHECK_GT(kMaxSize, gles2_context_provider_->GrContext()->maxTextureSize());
    return UseTooLargeImage() ? gfx::Size(10, kMaxSize) : gfx::Size(10, 10);
  }
};

TEST_F(OopPixelTest, DrawColor) {
  gfx::Rect rect(10, 10);
  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  display_item_list->push<DrawColorOp>(SK_ColorBLUE, SkBlendMode::kSrc);
  display_item_list->EndPaintOfUnpaired(rect);
  display_item_list->Finalize();

  std::vector<SkPMColor> expected_pixels(rect.width() * rect.height(),
                                         SkPreMultiplyARGB(255, 0, 0, 255));
  SkBitmap expected;
  expected.installPixels(
      SkImageInfo::MakeN32Premul(rect.width(), rect.height()),
      expected_pixels.data(), rect.width() * sizeof(SkColor));

  auto actual_oop = Raster(display_item_list, rect.size());
  ExpectEquals(actual_oop, expected, "oop");

  auto actual_gpu = RasterExpectedBitmap(display_item_list, rect.size());
  ExpectEquals(actual_gpu, expected, "gpu");
}

TEST_F(OopPixelTest, DrawColorWithTargetColorSpace) {
  gfx::Rect rect(10, 10);
  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  display_item_list->push<DrawColorOp>(SK_ColorBLUE, SkBlendMode::kSrc);
  display_item_list->EndPaintOfUnpaired(rect);
  display_item_list->Finalize();

  gfx::ColorSpace target_color_space = gfx::ColorSpace::CreateXYZD50();

  RasterOptions options(rect.size());
  options.color_space = target_color_space;

  auto actual = Raster(display_item_list, options);
  auto expected = RasterExpectedBitmap(display_item_list, options);
  ExpectEquals(actual, expected);

  // Verify conversion.
  EXPECT_EQ(SkColorSetARGB(255, 38, 15, 221), expected.getColor(0, 0));
}

TEST_F(OopPixelTest, DrawRect) {
  gfx::Rect rect(10, 10);
  auto color_paint = [](int r, int g, int b) {
    PaintFlags flags;
    flags.setColor(SkColorSetARGB(255, r, g, b));
    return flags;
  };
  std::vector<std::pair<SkRect, PaintFlags>> input = {
      {SkRect::MakeXYWH(0, 0, 5, 5), color_paint(0, 0, 255)},
      {SkRect::MakeXYWH(5, 0, 5, 5), color_paint(0, 255, 0)},
      {SkRect::MakeXYWH(0, 5, 5, 5), color_paint(0, 255, 255)},
      {SkRect::MakeXYWH(5, 5, 5, 5), color_paint(255, 0, 0)}};

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  for (auto& op : input) {
    display_item_list->StartPaint();
    display_item_list->push<DrawRectOp>(op.first, op.second);
    display_item_list->EndPaintOfUnpaired(
        gfx::ToEnclosingRect(gfx::SkRectToRectF(op.first)));
  }
  display_item_list->Finalize();

  auto actual = Raster(std::move(display_item_list), rect.size());

  // Expected colors are 5x5 rects of
  //  BLUE GREEN
  //  CYAN  RED
  std::vector<SkPMColor> expected_pixels(rect.width() * rect.height());
  for (int h = 0; h < rect.height(); ++h) {
    auto start = expected_pixels.begin() + h * rect.width();
    SkPMColor left_color = SkPreMultiplyColor(
        h < 5 ? input[0].second.getColor() : input[2].second.getColor());
    SkPMColor right_color = SkPreMultiplyColor(
        h < 5 ? input[1].second.getColor() : input[3].second.getColor());

    std::fill(start, start + 5, left_color);
    std::fill(start + 5, start + 10, right_color);
  }
  SkBitmap expected;
  expected.installPixels(
      SkImageInfo::MakeN32Premul(rect.width(), rect.height()),
      expected_pixels.data(), rect.width() * sizeof(SkPMColor));
  ExpectEquals(actual, expected);
}

TEST_P(OopImagePixelTest, DrawImage) {
  SCOPED_TRACE(base::StringPrintf("UseTooLargeImage: %d, FilterQuality: %d\n",
                                  UseTooLargeImage(), FilterQuality()));

  gfx::Rect rect(10, 10);
  gfx::Size image_size = GetImageSize();

  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(image_size.width(), image_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  SkCanvas canvas(bitmap);
  canvas.drawColor(SK_ColorMAGENTA);
  SkPaint green;
  green.setColor(SK_ColorGREEN);
  canvas.drawRect(SkRect::MakeXYWH(1, 2, 3, 4), green);

  sk_sp<SkImage> image = SkImage::MakeFromBitmap(bitmap);
  const PaintImage::Id kSomeId = 32;
  auto builder =
      PaintImageBuilder::WithDefault().set_image(image, 0).set_id(kSomeId);
  auto paint_image = builder.TakePaintImage();

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  PaintFlags flags;
  flags.setFilterQuality(FilterQuality());
  display_item_list->push<DrawImageOp>(paint_image, 0.f, 0.f, &flags);
  display_item_list->EndPaintOfUnpaired(rect);
  display_item_list->Finalize();

  auto actual = Raster(display_item_list, rect.size());
  auto expected = RasterExpectedBitmap(display_item_list, rect.size());
  ExpectEquals(actual, expected);

  EXPECT_EQ(actual.getColor(0, 0), SK_ColorMAGENTA);
}

TEST_P(OopImagePixelTest, DrawImageScaled) {
  SCOPED_TRACE(base::StringPrintf("UseTooLargeImage: %d, FilterQuality: %d\n",
                                  UseTooLargeImage(), FilterQuality()));

  gfx::Rect rect(10, 10);
  gfx::Size image_size = GetImageSize();

  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(image_size.width(), image_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  SkCanvas canvas(bitmap);
  canvas.drawColor(SK_ColorMAGENTA);
  SkPaint green;
  green.setColor(SK_ColorGREEN);
  canvas.drawRect(SkRect::MakeXYWH(1, 2, 3, 4), green);

  sk_sp<SkImage> image = SkImage::MakeFromBitmap(bitmap);
  auto builder = PaintImageBuilder::WithDefault().set_image(image, 0).set_id(
      PaintImage::GetNextId());
  auto paint_image = builder.TakePaintImage();

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  display_item_list->push<ScaleOp>(0.5f, 0.5f);
  PaintFlags flags;
  flags.setFilterQuality(FilterQuality());
  display_item_list->push<DrawImageOp>(paint_image, 0.f, 0.f, &flags);
  display_item_list->EndPaintOfUnpaired(rect);
  display_item_list->Finalize();

  auto actual = Raster(display_item_list, rect.size());
  auto expected = RasterExpectedBitmap(display_item_list, rect.size());
  ExpectEquals(actual, expected);
}

TEST_P(OopImagePixelTest, DrawImageShaderScaled) {
  SCOPED_TRACE(base::StringPrintf("UseTooLargeImage: %d, FilterQuality: %d\n",
                                  UseTooLargeImage(), FilterQuality()));

  gfx::Rect rect(10, 10);
  gfx::Size image_size = GetImageSize();

  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(image_size.width(), image_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  SkCanvas canvas(bitmap);
  canvas.drawColor(SK_ColorMAGENTA);
  SkPaint green;
  green.setColor(SK_ColorGREEN);
  canvas.drawRect(SkRect::MakeXYWH(1, 2, 3, 4), green);

  sk_sp<SkImage> image = SkImage::MakeFromBitmap(bitmap);
  auto builder = PaintImageBuilder::WithDefault().set_image(image, 0).set_id(
      PaintImage::GetNextId());
  auto paint_image = builder.TakePaintImage();
  auto paint_image_shader =
      PaintShader::MakeImage(paint_image, SkShader::kRepeat_TileMode,
                             SkShader::kRepeat_TileMode, nullptr);

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  display_item_list->push<ScaleOp>(0.5f, 0.5f);
  PaintFlags flags;
  flags.setShader(paint_image_shader);
  flags.setFilterQuality(FilterQuality());
  display_item_list->push<DrawRectOp>(gfx::RectToSkRect(rect), flags);
  display_item_list->EndPaintOfUnpaired(rect);
  display_item_list->Finalize();

  auto actual = Raster(display_item_list, rect.size());
  auto expected = RasterExpectedBitmap(display_item_list, rect.size());
  ExpectEquals(actual, expected);
}

TEST_P(OopImagePixelTest, DrawRecordShaderWithImageScaled) {
  SCOPED_TRACE(base::StringPrintf("UseTooLargeImage: %d, FilterQuality: %d\n",
                                  UseTooLargeImage(), FilterQuality()));

  gfx::Rect rect(10, 10);
  gfx::Size image_size = GetImageSize();

  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(image_size.width(), image_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  SkCanvas canvas(bitmap);
  canvas.drawColor(SK_ColorMAGENTA);
  SkPaint green;
  green.setColor(SK_ColorGREEN);
  canvas.drawRect(SkRect::MakeXYWH(1, 2, 3, 4), green);

  sk_sp<SkImage> image = SkImage::MakeFromBitmap(bitmap);
  auto builder = PaintImageBuilder::WithDefault().set_image(image, 0).set_id(
      PaintImage::GetNextId());
  auto paint_image = builder.TakePaintImage();
  auto paint_record = sk_make_sp<PaintOpBuffer>();
  PaintFlags flags;
  flags.setFilterQuality(FilterQuality());
  paint_record->push<DrawImageOp>(paint_image, 0.f, 0.f, &flags);
  auto paint_record_shader = PaintShader::MakePaintRecord(
      paint_record, gfx::RectToSkRect(rect), SkShader::kRepeat_TileMode,
      SkShader::kRepeat_TileMode, nullptr);
  // Set the shader has animated images so gpu also goes through cc's image
  // upload stack, instead of using skia.
  paint_record_shader->set_has_animated_images(true);

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  display_item_list->push<ScaleOp>(0.5f, 0.5f);
  PaintFlags raster_flags;
  raster_flags.setShader(paint_record_shader);
  raster_flags.setFilterQuality(FilterQuality());
  display_item_list->push<DrawRectOp>(gfx::RectToSkRect(rect), raster_flags);
  display_item_list->EndPaintOfUnpaired(rect);
  display_item_list->Finalize();

  auto actual = Raster(display_item_list, rect.size());
  auto expected = RasterExpectedBitmap(display_item_list, rect.size());
  ExpectEquals(actual, expected);
}

TEST_P(OopImagePixelTest, DrawImageWithTargetColorSpace) {
  SCOPED_TRACE(base::StringPrintf("UseTooLargeImage: %d, FilterQuality: %d\n",
                                  UseTooLargeImage(), FilterQuality()));

  gfx::Rect rect(10, 10);
  gfx::Size image_size = GetImageSize();

  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(image_size.width(), image_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  SkCanvas canvas(bitmap);
  canvas.drawColor(SK_ColorMAGENTA);
  SkPaint green;
  green.setColor(SK_ColorGREEN);
  canvas.drawRect(SkRect::MakeXYWH(1, 2, 3, 4), green);

  sk_sp<SkImage> image = SkImage::MakeFromBitmap(bitmap);
  const PaintImage::Id kSomeId = 32;
  auto builder =
      PaintImageBuilder::WithDefault().set_image(image, 0).set_id(kSomeId);
  auto paint_image = builder.TakePaintImage();

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  PaintFlags flags;
  flags.setFilterQuality(FilterQuality());
  display_item_list->push<DrawImageOp>(paint_image, 0.f, 0.f, &flags);
  display_item_list->EndPaintOfUnpaired(rect);
  display_item_list->Finalize();

  RasterOptions options(rect.size());
  options.color_space = gfx::ColorSpace::CreateDisplayP3D65();

  auto actual = Raster(display_item_list, options);
  auto expected = RasterExpectedBitmap(display_item_list, options);
  ExpectEquals(actual, expected);

  // Verify some conversion occurred here and that actual != bitmap.
  EXPECT_NE(actual.getColor(0, 0), SK_ColorMAGENTA);
}

TEST_P(OopImagePixelTest, DrawImageWithSourceColorSpace) {
  SCOPED_TRACE(base::StringPrintf("UseTooLargeImage: %d, FilterQuality: %d\n",
                                  UseTooLargeImage(), FilterQuality()));

  gfx::Rect rect(10, 10);
  gfx::Size image_size = GetImageSize();

  auto color_space = gfx::ColorSpace::CreateDisplayP3D65().ToSkColorSpace();
  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(image_size.width(), image_size.height(),
                                 color_space),
      SkBitmap::kZeroPixels_AllocFlag);

  SkCanvas canvas(bitmap);
  canvas.drawColor(SK_ColorMAGENTA);
  SkPaint green;
  green.setColor(SK_ColorGREEN);
  canvas.drawRect(SkRect::MakeXYWH(1, 2, 3, 4), green);

  sk_sp<SkImage> image = SkImage::MakeFromBitmap(bitmap);
  const PaintImage::Id kSomeId = 32;
  auto builder =
      PaintImageBuilder::WithDefault().set_image(image, 0).set_id(kSomeId);
  auto paint_image = builder.TakePaintImage();
  EXPECT_EQ(paint_image.color_space(), color_space.get());

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  PaintFlags flags;
  flags.setFilterQuality(FilterQuality());
  display_item_list->push<DrawImageOp>(paint_image, 0.f, 0.f, &flags);
  display_item_list->EndPaintOfUnpaired(rect);
  display_item_list->Finalize();

  RasterOptions options(rect.size());

  auto actual = Raster(display_item_list, options);
  auto expected = RasterExpectedBitmap(display_item_list, options);
  ExpectEquals(actual, expected);

  // Colors get converted when being drawn to the bitmap.
  EXPECT_NE(bitmap.getColor(0, 0), SK_ColorMAGENTA);
}

TEST_P(OopImagePixelTest, DrawImageWithSourceAndTargetColorSpace) {
  SCOPED_TRACE(base::StringPrintf("UseTooLargeImage: %d, FilterQuality: %d\n",
                                  UseTooLargeImage(), FilterQuality()));

  gfx::Rect rect(10, 10);

  gfx::Size image_size = GetImageSize();
  auto color_space = gfx::ColorSpace::CreateXYZD50().ToSkColorSpace();
  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(image_size.width(), image_size.height(),
                                 color_space),
      SkBitmap::kZeroPixels_AllocFlag);

  SkCanvas canvas(bitmap);
  canvas.drawColor(SK_ColorMAGENTA);
  SkPaint green;
  green.setColor(SK_ColorGREEN);
  canvas.drawRect(SkRect::MakeXYWH(1, 2, 3, 4), green);

  sk_sp<SkImage> image = SkImage::MakeFromBitmap(bitmap);
  const PaintImage::Id kSomeId = 32;
  auto builder =
      PaintImageBuilder::WithDefault().set_image(image, 0).set_id(kSomeId);
  auto paint_image = builder.TakePaintImage();
  EXPECT_EQ(paint_image.color_space(), color_space.get());

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  PaintFlags flags;
  flags.setFilterQuality(FilterQuality());
  display_item_list->push<DrawImageOp>(paint_image, 0.f, 0.f, &flags);
  display_item_list->EndPaintOfUnpaired(rect);
  display_item_list->Finalize();

  RasterOptions options(rect.size());
  options.color_space = gfx::ColorSpace::CreateDisplayP3D65();

  auto actual = Raster(display_item_list, options);
  auto expected = RasterExpectedBitmap(display_item_list, options);
  ExpectEquals(actual, expected);
}

TEST_F(OopPixelTest, Preclear) {
  gfx::Rect rect(10, 10);
  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->Finalize();

  RasterOptions options;
  options.resource_size = rect.size();
  options.full_raster_rect = rect;
  options.playback_rect = rect;
  options.background_color = SK_ColorMAGENTA;
  options.preclear = true;
  options.preclear_color = SK_ColorGREEN;

  auto actual = Raster(display_item_list, options);

  options.preclear = false;
  options.background_color = SK_ColorGREEN;
  auto expected = RasterExpectedBitmap(display_item_list, options);
  ExpectEquals(actual, expected);
}

TEST_F(OopPixelTest, ClearingOpaqueCorner) {
  // Verify that clears work properly for both the right and bottom sides
  // of an opaque corner tile.

  RasterOptions options;
  gfx::Point arbitrary_offset(10, 20);
  options.resource_size = gfx::Size(10, 10);
  options.full_raster_rect = gfx::Rect(arbitrary_offset, gfx::Size(8, 7));
  options.content_size = gfx::Size(options.full_raster_rect.right(),
                                   options.full_raster_rect.bottom());
  options.playback_rect = options.full_raster_rect;
  options.background_color = SK_ColorGREEN;
  float arbitrary_scale = 0.25f;
  options.post_scale = arbitrary_scale;
  options.requires_clear = false;
  options.preclear = true;
  options.preclear_color = SK_ColorRED;

  // Make a non-empty but noop display list to avoid early outs.
  auto display_item_list = MakeNoopDisplayItemList();

  auto oop_result = Raster(display_item_list, options);
  auto gpu_result = RasterExpectedBitmap(display_item_list, options);

  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(options.resource_size.width(),
                                 options.resource_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  // Expect a two pixel border from texels 7-9 on the column and 6-8 on row.
  SkCanvas canvas(bitmap);
  canvas.drawColor(options.preclear_color);
  SkPaint green;
  green.setColor(options.background_color);
  canvas.drawRect(SkRect::MakeXYWH(7, 0, 2, 8), green);
  canvas.drawRect(SkRect::MakeXYWH(0, 6, 9, 2), green);

  ExpectEquals(oop_result, bitmap, "oop");
  ExpectEquals(gpu_result, bitmap, "gpu");
}

TEST_F(OopPixelTest, ClearingOpaqueCornerExactEdge) {
  // Verify that clears work properly for both the right and bottom sides
  // of an opaque corner tile whose content rect exactly lines up with
  // the edge of the resource.

  RasterOptions options;
  gfx::Point arbitrary_offset(10, 20);
  options.resource_size = gfx::Size(10, 10);
  options.full_raster_rect = gfx::Rect(arbitrary_offset, options.resource_size);
  options.content_size = gfx::Size(options.full_raster_rect.right(),
                                   options.full_raster_rect.bottom());
  options.playback_rect = options.full_raster_rect;
  options.background_color = SK_ColorGREEN;
  float arbitrary_scale = 0.25f;
  options.post_scale = arbitrary_scale;
  options.requires_clear = false;
  options.preclear = true;
  options.preclear_color = SK_ColorRED;

  // Make a non-empty but noop display list to avoid early outs.
  auto display_item_list = MakeNoopDisplayItemList();

  auto oop_result = Raster(display_item_list, options);
  auto gpu_result = RasterExpectedBitmap(display_item_list, options);

  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(options.resource_size.width(),
                                 options.resource_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  // Expect a one pixel border on the bottom/right edge.
  SkCanvas canvas(bitmap);
  canvas.drawColor(options.preclear_color);
  SkPaint green;
  green.setColor(options.background_color);
  canvas.drawRect(SkRect::MakeXYWH(9, 0, 1, 10), green);
  canvas.drawRect(SkRect::MakeXYWH(0, 9, 10, 1), green);

  ExpectEquals(oop_result, bitmap, "oop");
  ExpectEquals(gpu_result, bitmap, "gpu");
}

TEST_F(OopPixelTest, ClearingOpaqueCornerPartialRaster) {
  // Verify that clears do nothing on an opaque corner tile whose
  // partial raster rect doesn't intersect the edge of the content.

  RasterOptions options;
  options.resource_size = gfx::Size(10, 10);
  gfx::Point arbitrary_offset(30, 12);
  options.full_raster_rect = gfx::Rect(arbitrary_offset, gfx::Size(8, 7));
  options.content_size = gfx::Size(options.full_raster_rect.right(),
                                   options.full_raster_rect.bottom());
  options.playback_rect =
      gfx::Rect(arbitrary_offset.x() + 5, arbitrary_offset.y() + 3, 2, 4);
  options.background_color = SK_ColorGREEN;
  options.requires_clear = false;
  options.preclear = true;
  options.preclear_color = SK_ColorRED;

  // Make a non-empty but noop display list to avoid early outs.
  auto display_item_list = MakeNoopDisplayItemList();

  auto oop_result = Raster(display_item_list, options);
  auto gpu_result = RasterExpectedBitmap(display_item_list, options);

  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(options.resource_size.width(),
                                 options.resource_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  // Expect no clearing here because the playback rect is internal.
  SkCanvas canvas(bitmap);
  canvas.drawColor(options.preclear_color);

  ExpectEquals(oop_result, bitmap, "oop");
  ExpectEquals(gpu_result, bitmap, "gpu");
}

TEST_F(OopPixelTest, ClearingOpaqueRightEdge) {
  // Verify that a tile that intersects the right edge of content
  // but not the bottom only clears the right pixels.

  RasterOptions options;
  gfx::Point arbitrary_offset(30, 40);
  options.resource_size = gfx::Size(10, 10);
  options.full_raster_rect = gfx::Rect(arbitrary_offset, gfx::Size(3, 10));
  options.content_size = gfx::Size(options.full_raster_rect.right(),
                                   options.full_raster_rect.bottom() + 1000);
  options.playback_rect = options.full_raster_rect;
  options.background_color = SK_ColorGREEN;
  options.requires_clear = false;
  options.preclear = true;
  options.preclear_color = SK_ColorRED;

  // Make a non-empty but noop display list to avoid early outs.
  auto display_item_list = MakeNoopDisplayItemList();

  auto oop_result = Raster(display_item_list, options);
  auto gpu_result = RasterExpectedBitmap(display_item_list, options);

  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(options.resource_size.width(),
                                 options.resource_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  // Expect a two pixel column border from texels 2-4.
  SkCanvas canvas(bitmap);
  canvas.drawColor(options.preclear_color);
  SkPaint green;
  green.setColor(options.background_color);
  canvas.drawRect(SkRect::MakeXYWH(2, 0, 2, 10), green);

  ExpectEquals(oop_result, bitmap, "oop");
  ExpectEquals(gpu_result, bitmap, "gpu");
}

TEST_F(OopPixelTest, ClearingOpaqueBottomEdge) {
  // Verify that a tile that intersects the bottom edge of content
  // but not the right only clears the bottom pixels.

  RasterOptions options;
  gfx::Point arbitrary_offset(10, 20);
  options.resource_size = gfx::Size(10, 10);
  options.full_raster_rect = gfx::Rect(arbitrary_offset, gfx::Size(10, 5));
  options.content_size = gfx::Size(options.full_raster_rect.right() + 1000,
                                   options.full_raster_rect.bottom());
  options.playback_rect = options.full_raster_rect;
  options.background_color = SK_ColorGREEN;
  float arbitrary_scale = 0.25f;
  options.post_scale = arbitrary_scale;
  options.requires_clear = false;
  options.preclear = true;
  options.preclear_color = SK_ColorRED;

  // Make a non-empty but noop display list to avoid early outs.
  auto display_item_list = MakeNoopDisplayItemList();

  auto oop_result = Raster(display_item_list, options);
  auto gpu_result = RasterExpectedBitmap(display_item_list, options);

  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(options.resource_size.width(),
                                 options.resource_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  // Expect a two pixel border from texels 4-6 on the row
  SkCanvas canvas(bitmap);
  canvas.drawColor(options.preclear_color);
  SkPaint green;
  green.setColor(options.background_color);
  canvas.drawRect(SkRect::MakeXYWH(0, 4, 10, 2), green);

  ExpectEquals(oop_result, bitmap, "oop");
  ExpectEquals(gpu_result, bitmap, "gpu");
}

TEST_F(OopPixelTest, ClearingOpaqueInternal) {
  // Verify that an internal opaque tile does no clearing.

  RasterOptions options;
  gfx::Point arbitrary_offset(35, 12);
  options.resource_size = gfx::Size(10, 10);
  options.full_raster_rect = gfx::Rect(arbitrary_offset, options.resource_size);
  // Very large content rect to make this an internal tile.
  options.content_size = gfx::Size(1000, 1000);
  options.playback_rect = options.full_raster_rect;
  options.background_color = SK_ColorGREEN;
  float arbitrary_scale = 1.2345f;
  options.post_scale = arbitrary_scale;
  options.requires_clear = false;
  options.preclear = true;
  options.preclear_color = SK_ColorRED;

  // Make a non-empty but noop display list to avoid early outs.
  auto display_item_list = MakeNoopDisplayItemList();

  auto oop_result = Raster(display_item_list, options);
  auto gpu_result = RasterExpectedBitmap(display_item_list, options);

  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(options.resource_size.width(),
                                 options.resource_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  // Expect no clears here, as this tile does not intersect the edge of the
  // tile.
  SkCanvas canvas(bitmap);
  canvas.drawColor(options.preclear_color);

  ExpectEquals(oop_result, bitmap, "oop");
  ExpectEquals(gpu_result, bitmap, "gpu");
}

TEST_F(OopPixelTest, ClearingTransparentCorner) {
  RasterOptions options;
  gfx::Point arbitrary_offset(5, 8);
  options.resource_size = gfx::Size(10, 10);
  options.full_raster_rect = gfx::Rect(arbitrary_offset, gfx::Size(8, 7));
  options.content_size = gfx::Size(options.full_raster_rect.right(),
                                   options.full_raster_rect.bottom());
  options.playback_rect = options.full_raster_rect;
  options.background_color = SK_ColorTRANSPARENT;
  float arbitrary_scale = 3.7f;
  options.post_scale = arbitrary_scale;
  options.requires_clear = true;
  options.preclear = true;
  options.preclear_color = SK_ColorRED;

  // Make a non-empty but noop display list to avoid early outs.
  auto display_item_list = MakeNoopDisplayItemList();

  auto oop_result = Raster(display_item_list, options);
  auto gpu_result = RasterExpectedBitmap(display_item_list, options);

  // Because this is rastering the entire tile, clear the entire thing
  // even if the full raster rect doesn't cover the whole resource.
  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(options.resource_size.width(),
                                 options.resource_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  SkCanvas canvas(bitmap);
  canvas.drawColor(SK_ColorTRANSPARENT);

  ExpectEquals(oop_result, bitmap, "oop");
  ExpectEquals(gpu_result, bitmap, "gpu");
}

TEST_F(OopPixelTest, ClearingTransparentInternalTile) {
  // Content rect much larger than full raster rect or playback rect.
  // This should still clear the tile.
  RasterOptions options;
  gfx::Point arbitrary_offset(100, 200);
  options.resource_size = gfx::Size(10, 10);
  options.full_raster_rect = gfx::Rect(arbitrary_offset, options.resource_size);
  options.content_size = gfx::Size(1000, 1000);
  options.playback_rect = options.full_raster_rect;
  options.background_color = SK_ColorTRANSPARENT;
  float arbitrary_scale = 3.7f;
  options.post_scale = arbitrary_scale;
  options.requires_clear = true;
  options.preclear = true;
  options.preclear_color = SK_ColorRED;

  // Make a non-empty but noop display list to avoid early outs.
  auto display_item_list = MakeNoopDisplayItemList();

  auto oop_result = Raster(display_item_list, options);
  auto gpu_result = RasterExpectedBitmap(display_item_list, options);

  // Because this is rastering the entire tile, clear the entire thing
  // even if the full raster rect doesn't cover the whole resource.
  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(options.resource_size.width(),
                                 options.resource_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  SkCanvas canvas(bitmap);
  canvas.drawColor(SK_ColorTRANSPARENT);

  ExpectEquals(oop_result, bitmap, "oop");
  ExpectEquals(gpu_result, bitmap, "gpu");
}

TEST_F(OopPixelTest, ClearingTransparentCornerPartialRaster) {
  RasterOptions options;
  options.resource_size = gfx::Size(10, 10);
  gfx::Point arbitrary_offset(30, 12);
  options.full_raster_rect = gfx::Rect(arbitrary_offset, gfx::Size(8, 7));
  options.content_size = gfx::Size(options.full_raster_rect.right(),
                                   options.full_raster_rect.bottom());
  options.playback_rect =
      gfx::Rect(arbitrary_offset.x() + 5, arbitrary_offset.y() + 3, 2, 4);
  options.background_color = SK_ColorTRANSPARENT;
  float arbitrary_scale = 0.23f;
  options.post_scale = arbitrary_scale;
  options.requires_clear = true;
  options.preclear = true;
  options.preclear_color = SK_ColorRED;

  // Make a non-empty but noop display list to avoid early outs.
  auto display_item_list = MakeNoopDisplayItemList();

  auto oop_result = Raster(display_item_list, options);
  auto gpu_result = RasterExpectedBitmap(display_item_list, options);

  SkBitmap bitmap;
  bitmap.allocPixelsFlags(
      SkImageInfo::MakeN32Premul(options.resource_size.width(),
                                 options.resource_size.height()),
      SkBitmap::kZeroPixels_AllocFlag);

  // Result should be a red background with a cleared hole where the
  // playback_rect is.
  SkCanvas canvas(bitmap);
  canvas.drawColor(options.preclear_color);
  canvas.translate(-arbitrary_offset.x(), -arbitrary_offset.y());
  canvas.clipRect(gfx::RectToSkRect(options.playback_rect));
  canvas.drawColor(SK_ColorTRANSPARENT, SkBlendMode::kSrc);

  ExpectEquals(oop_result, bitmap, "oop");
  ExpectEquals(gpu_result, bitmap, "gpu");
}

// Test various bitmap and playback rects in the raster options, to verify
// that in process (RasterSource) and out of process (GLES2Implementation)
// raster behave identically.
TEST_F(OopPixelTest, DrawRectBasicRasterOptions) {
  PaintFlags flags;
  flags.setColor(SkColorSetARGB(255, 250, 10, 20));
  gfx::Rect draw_rect(3, 1, 8, 9);

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  display_item_list->push<DrawRectOp>(gfx::RectToSkRect(draw_rect), flags);
  display_item_list->EndPaintOfUnpaired(draw_rect);
  display_item_list->Finalize();

  std::vector<std::pair<gfx::Rect, gfx::Rect>> input = {
      {{0, 0, 10, 10}, {0, 0, 10, 10}},
      {{1, 2, 10, 10}, {4, 2, 5, 6}},
      {{5, 5, 15, 10}, {5, 5, 10, 10}}};

  for (size_t i = 0; i < input.size(); ++i) {
    SCOPED_TRACE(base::StringPrintf("Case %zd", i));

    RasterOptions options;
    options.resource_size = input[i].first.size(),
    options.full_raster_rect = input[i].first;
    options.content_size = gfx::Size(options.full_raster_rect.right(),
                                     options.full_raster_rect.bottom());
    options.playback_rect = input[i].second;
    options.background_color = SK_ColorMAGENTA;

    auto actual = Raster(display_item_list, options);
    auto expected = RasterExpectedBitmap(display_item_list, options);
    ExpectEquals(actual, expected);
  }
}

TEST_F(OopPixelTest, DrawRectScaleTransformOptions) {
  PaintFlags flags;
  // Use powers of two here to make floating point blending consistent.
  flags.setColor(SkColorSetARGB(128, 64, 128, 32));
  flags.setAntiAlias(true);
  gfx::Rect draw_rect(3, 4, 8, 9);

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  display_item_list->push<DrawRectOp>(gfx::RectToSkRect(draw_rect), flags);
  display_item_list->EndPaintOfUnpaired(draw_rect);
  display_item_list->Finalize();

  // Draw a greenish transparent box, partially offset and clipped in the
  // bottom right.  It should appear near the upper left of a cyan background,
  // with the left and top sides of the greenish box partially blended due to
  // the post translate.
  RasterOptions options;
  options.resource_size = {20, 20};
  options.content_size = {25, 25};
  options.full_raster_rect = {5, 5, 20, 20};
  options.playback_rect = {5, 5, 13, 9};
  options.background_color = SK_ColorCYAN;
  options.post_translate = {0.5f, 0.25f};
  options.post_scale = 2.f;

  auto actual = Raster(display_item_list, options);
  auto expected = RasterExpectedBitmap(display_item_list, options);
  ExpectEquals(actual, expected);
}

TEST_F(OopPixelTest, DrawRectQueryMiddleOfDisplayList) {
  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  std::vector<SkColor> colors = {
      SkColorSetARGB(255, 0, 0, 255),    SkColorSetARGB(255, 0, 255, 0),
      SkColorSetARGB(255, 0, 255, 255),  SkColorSetARGB(255, 255, 0, 0),
      SkColorSetARGB(255, 255, 0, 255),  SkColorSetARGB(255, 255, 255, 0),
      SkColorSetARGB(255, 255, 255, 255)};

  for (int i = 0; i < 20; ++i) {
    gfx::Rect draw_rect(0, i, 1, 1);
    PaintFlags flags;
    flags.setColor(colors[i % colors.size()]);
    display_item_list->StartPaint();
    display_item_list->push<DrawRectOp>(gfx::RectToSkRect(draw_rect), flags);
    display_item_list->EndPaintOfUnpaired(draw_rect);
  }
  display_item_list->Finalize();

  // Draw a "tile" in the middle of the display list with a post scale.
  RasterOptions options;
  options.resource_size = {10, 10};
  options.content_size = {20, 20};
  options.full_raster_rect = {0, 10, 1, 10};
  options.playback_rect = {0, 10, 1, 10};
  options.background_color = SK_ColorGRAY;
  options.post_translate = {0.f, 0.f};
  options.post_scale = 2.f;

  auto actual = Raster(display_item_list, options);
  auto expected = RasterExpectedBitmap(display_item_list, options);
  ExpectEquals(actual, expected);
}

TEST_F(OopPixelTest, DrawRectColorSpace) {
  RasterOptions options;
  options.resource_size = gfx::Size(100, 100);
  options.content_size = options.resource_size;
  options.full_raster_rect = gfx::Rect(options.content_size);
  options.playback_rect = options.full_raster_rect;
  options.color_space = gfx::ColorSpace::CreateDisplayP3D65();

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  PaintFlags flags;
  flags.setStyle(PaintFlags::kFill_Style);
  flags.setColor(SK_ColorGREEN);
  display_item_list->push<DrawRectOp>(
      gfx::RectToSkRect(gfx::Rect(options.resource_size)), flags);
  display_item_list->EndPaintOfUnpaired(options.full_raster_rect);
  display_item_list->Finalize();

  auto actual = Raster(display_item_list, options);
  auto expected = RasterExpectedBitmap(display_item_list, options);
  ExpectEquals(actual, expected);
}

scoped_refptr<PaintTextBlob> buildTextBlob() {
  SkFontStyle style;
  PaintTypeface typeface =
      PaintTypeface::FromFamilyNameAndFontStyle("monospace", style);

  PaintFont font;
  font.SetTypeface(typeface);
  font.SetTextEncoding(SkPaint::kGlyphID_TextEncoding);
  font.SetHinting(SkPaint::kNormal_Hinting);
  font.SetTextSize(10u);

  PaintTextBlobBuilder builder;
  SkRect bounds = SkRect::MakeWH(100, 100);
  const int glyphCount = 10;
  const auto& runBuffer = builder.AllocRunPosH(font, glyphCount, 0, &bounds);
  for (int i = 0; i < glyphCount; i++) {
    runBuffer.glyphs[i] = static_cast<SkGlyphID>(i);
    runBuffer.pos[i] = SkIntToScalar(i);
  }
  return builder.TakeTextBlob();
}

TEST_F(OopPixelTest, DrawTextBlob) {
  RasterOptions options;
  options.resource_size = gfx::Size(100, 100);
  options.content_size = options.resource_size;
  options.full_raster_rect = gfx::Rect(options.content_size);
  options.playback_rect = options.full_raster_rect;
  options.color_space = gfx::ColorSpace::CreateSRGB();

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  PaintFlags flags;
  flags.setStyle(PaintFlags::kFill_Style);
  flags.setColor(SK_ColorGREEN);
  display_item_list->push<DrawTextBlobOp>(buildTextBlob(), 0u, 0u, flags);
  display_item_list->EndPaintOfUnpaired(options.full_raster_rect);
  display_item_list->Finalize();

  auto actual = Raster(display_item_list, options);
  auto expected = RasterExpectedBitmap(display_item_list, options);
  ExpectEquals(actual, expected);
}

TEST_F(OopPixelTest, DrawRecordShaderWithTextScaled) {
  RasterOptions options;
  options.resource_size = gfx::Size(100, 100);
  options.content_size = options.resource_size;
  options.full_raster_rect = gfx::Rect(options.content_size);
  options.playback_rect = options.full_raster_rect;
  options.color_space = gfx::ColorSpace::CreateSRGB();

  auto paint_record = sk_make_sp<PaintOpBuffer>();
  PaintFlags flags;
  flags.setStyle(PaintFlags::kFill_Style);
  flags.setColor(SK_ColorGREEN);
  paint_record->push<DrawTextBlobOp>(buildTextBlob(), 0u, 0u, flags);
  auto paint_record_shader = PaintShader::MakePaintRecord(
      paint_record, SkRect::MakeWH(25, 25), SkShader::kRepeat_TileMode,
      SkShader::kRepeat_TileMode, nullptr);

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  display_item_list->push<ScaleOp>(2.f, 2.f);
  PaintFlags shader_flags;
  shader_flags.setShader(paint_record_shader);
  display_item_list->push<DrawRectOp>(SkRect::MakeWH(50, 50), shader_flags);
  display_item_list->EndPaintOfUnpaired(options.full_raster_rect);
  display_item_list->Finalize();

  auto actual = Raster(display_item_list, options);
  auto expected = RasterExpectedBitmap(display_item_list, options);
  ExpectEquals(actual, expected);
}

TEST_F(OopPixelTest, DrawRecordFilterWithTextScaled) {
  RasterOptions options;
  options.resource_size = gfx::Size(100, 100);
  options.content_size = options.resource_size;
  options.full_raster_rect = gfx::Rect(options.content_size);
  options.playback_rect = options.full_raster_rect;
  options.color_space = gfx::ColorSpace::CreateSRGB();

  auto paint_record = sk_make_sp<PaintOpBuffer>();
  PaintFlags flags;
  flags.setStyle(PaintFlags::kFill_Style);
  flags.setColor(SK_ColorGREEN);
  paint_record->push<DrawTextBlobOp>(buildTextBlob(), 0u, 0u, flags);
  auto paint_record_filter =
      sk_make_sp<RecordPaintFilter>(paint_record, SkRect::MakeWH(100, 100));

  auto display_item_list = base::MakeRefCounted<DisplayItemList>();
  display_item_list->StartPaint();
  display_item_list->push<ScaleOp>(2.f, 2.f);
  PaintFlags shader_flags;
  shader_flags.setImageFilter(paint_record_filter);
  display_item_list->push<DrawRectOp>(SkRect::MakeWH(50, 50), shader_flags);
  display_item_list->EndPaintOfUnpaired(options.full_raster_rect);
  display_item_list->Finalize();

  auto actual = Raster(display_item_list, options);
  auto expected = RasterExpectedBitmap(display_item_list, options);
  ExpectEquals(actual, expected);
}

INSTANTIATE_TEST_CASE_P(P, OopImagePixelTest, ::testing::Bool());

}  // namespace
}  // namespace cc

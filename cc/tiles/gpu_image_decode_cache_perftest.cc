// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "cc/base/lap_timer.h"
#include "cc/paint/draw_image.h"
#include "cc/paint/paint_image_builder.h"
#include "cc/raster/tile_task.h"
#include "cc/test/test_in_process_context_provider.h"
#include "cc/tiles/gpu_image_decode_cache.h"
#include "gpu/command_buffer/client/raster_interface.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace cc {
namespace {

static const int kTimeLimitMillis = 2000;
static const int kWarmupRuns = 5;
static const int kTimeCheckInterval = 10;
static const int kCacheSize = 128 * 1024 * 1024;

sk_sp<SkImage> CreateImage(int width, int height) {
  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::MakeS32(width, height, kPremul_SkAlphaType));
  return SkImage::MakeFromBitmap(bitmap);
}

SkMatrix CreateMatrix(const SkSize& scale) {
  SkMatrix matrix;
  matrix.setScale(scale.width(), scale.height());
  return matrix;
}

enum class TestMode { kGpu, kTransferCache, kSw };

class GpuImageDecodeCachePerfTest
    : public testing::Test,
      public testing::WithParamInterface<TestMode> {
 public:
  GpuImageDecodeCachePerfTest()
      : timer_(kWarmupRuns,
               base::TimeDelta::FromMilliseconds(kTimeLimitMillis),
               kTimeCheckInterval),
        context_provider_(base::MakeRefCounted<TestInProcessContextProvider>(
            UseTransferCache(),
            false /* support_locking */)),
        cache_(context_provider_.get(),
               UseTransferCache(),
               kRGBA_8888_SkColorType,
               kCacheSize,
               MaxTextureSize()) {}

 protected:
  size_t MaxTextureSize() const {
    switch (GetParam()) {
      case TestMode::kGpu:
      case TestMode::kTransferCache:
        return 4096;
      case TestMode::kSw:
        return 0;
    }
  }

  bool UseTransferCache() const {
    return GetParam() == TestMode::kTransferCache;
  }

  const char* ParamName() const {
    switch (GetParam()) {
      case TestMode::kGpu:
        return "GPU";
      case TestMode::kTransferCache:
        return "TransferCache";
      case TestMode::kSw:
        return "SW";
    }
  }

  LapTimer timer_;
  scoped_refptr<TestInProcessContextProvider> context_provider_;
  GpuImageDecodeCache cache_;
};

INSTANTIATE_TEST_CASE_P(P,
                        GpuImageDecodeCachePerfTest,
                        testing::Values(TestMode::kGpu,
                                        TestMode::kTransferCache,
                                        TestMode::kSw));

TEST_P(GpuImageDecodeCachePerfTest, DecodeWithColorConversion) {
  timer_.Reset();
  do {
    DrawImage image(
        PaintImageBuilder::WithDefault()
            .set_id(PaintImage::GetNextId())
            .set_image(CreateImage(1024, 2048), PaintImage::GetNextContentId())
            .TakePaintImage(),
        SkIRect::MakeWH(1024, 2048), kMedium_SkFilterQuality,
        CreateMatrix(SkSize::Make(1.0f, 1.0f)), 0u,
        gfx::ColorSpace::CreateXYZD50());

    DecodedDrawImage decoded_image = cache_.GetDecodedImageForDraw(image);
    cache_.DrawWithImageFinished(image, decoded_image);
    timer_.NextLap();
  } while (!timer_.HasTimeLimitExpired());

  perf_test::PrintResult("gpu_image_decode_cache_decode_with_color_conversion",
                         ParamName(), "result", timer_.LapsPerSecond(),
                         "runs/s", true);
}

using GpuImageDecodeCachePerfTestNoSw = GpuImageDecodeCachePerfTest;
INSTANTIATE_TEST_CASE_P(P,
                        GpuImageDecodeCachePerfTestNoSw,
                        testing::Values(TestMode::kGpu,
                                        TestMode::kTransferCache));

TEST_P(GpuImageDecodeCachePerfTestNoSw, DecodeWithMips) {
  // Surface to render into.
  auto surface = SkSurface::MakeRenderTarget(
      context_provider_->GrContext(), SkBudgeted::kNo,
      SkImageInfo::MakeN32Premul(2048, 2048));

  timer_.Reset();
  do {
    DrawImage image(
        PaintImageBuilder::WithDefault()
            .set_id(PaintImage::GetNextId())
            .set_image(CreateImage(1024, 2048), PaintImage::GetNextContentId())
            .TakePaintImage(),
        SkIRect::MakeWH(1024, 2048), kMedium_SkFilterQuality,
        CreateMatrix(SkSize::Make(0.6f, 0.6f)), 0u, gfx::ColorSpace());

    DecodedDrawImage decoded_image = cache_.GetDecodedImageForDraw(image);

    if (GetParam() == TestMode::kGpu) {
      SkPaint paint;
      paint.setFilterQuality(kMedium_SkFilterQuality);
      surface->getCanvas()->drawImageRect(decoded_image.image().get(),
                                          SkRect::MakeWH(1024, 2048),
                                          SkRect::MakeWH(614, 1229), &paint);
      surface->prepareForExternalIO();
    }

    cache_.DrawWithImageFinished(image, decoded_image);
    timer_.NextLap();
  } while (!timer_.HasTimeLimitExpired());

  perf_test::PrintResult("gpu_image_decode_cache_decode_with_mips", ParamName(),
                         "result", timer_.LapsPerSecond(), "runs/s", true);
}

TEST_P(GpuImageDecodeCachePerfTest, AcquireExistingImages) {
  timer_.Reset();
  DrawImage image(
      PaintImageBuilder::WithDefault()
          .set_id(PaintImage::GetNextId())
          .set_image(CreateImage(1024, 2048), PaintImage::GetNextContentId())
          .TakePaintImage(),
      SkIRect::MakeWH(1024, 2048), kMedium_SkFilterQuality,
      CreateMatrix(SkSize::Make(1.0f, 1.0f)), 0u,
      gfx::ColorSpace::CreateXYZD50());

  DecodedDrawImage decoded_image = cache_.GetDecodedImageForDraw(image);
  cache_.DrawWithImageFinished(image, decoded_image);

  do {
    DecodedDrawImage decoded_image = cache_.GetDecodedImageForDraw(image);
    cache_.DrawWithImageFinished(image, decoded_image);
    timer_.NextLap();
  } while (!timer_.HasTimeLimitExpired());

  perf_test::PrintResult("gpu_image_decode_cache_acquire_existing_images",
                         ParamName(), "result", timer_.LapsPerSecond(),
                         "runs/s", true);
}

}  // namespace
}  // namespace cc

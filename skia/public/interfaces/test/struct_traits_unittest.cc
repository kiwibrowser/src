// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "skia/public/interfaces/test/traits_test_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkString.h"
#include "third_party/skia/include/effects/SkBlurImageFilter.h"
#include "third_party/skia/include/effects/SkColorFilterImageFilter.h"
#include "third_party/skia/include/effects/SkDropShadowImageFilter.h"
#include "ui/gfx/skia_util.h"

namespace skia {

namespace {

class StructTraitsTest : public testing::Test, public mojom::TraitsTestService {
 public:
  StructTraitsTest() = default;

 protected:
  mojom::TraitsTestServicePtr GetTraitsTestProxy() {
    mojom::TraitsTestServicePtr proxy;
    traits_test_bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

 private:
  // TraitsTestService:
  void EchoImageInfo(const SkImageInfo& i,
                     EchoImageInfoCallback callback) override {
    std::move(callback).Run(i);
  }

  void EchoBitmap(const SkBitmap& b, EchoBitmapCallback callback) override {
    std::move(callback).Run(b);
  }

  void EchoBlurImageFilterTileMode(
      SkBlurImageFilter::TileMode t,
      EchoBlurImageFilterTileModeCallback callback) override {
    std::move(callback).Run(t);
  }

  base::MessageLoop loop_;
  mojo::BindingSet<TraitsTestService> traits_test_bindings_;

  DISALLOW_COPY_AND_ASSIGN(StructTraitsTest);
};

static bool colorspace_srgb_gamma(SkColorSpace* cs) {
  return cs && cs->gammaCloseToSRGB();
}

}  // namespace

TEST_F(StructTraitsTest, ImageInfo) {
  SkImageInfo input = SkImageInfo::Make(
      34, 56, SkColorType::kGray_8_SkColorType,
      SkAlphaType::kUnpremul_SkAlphaType, SkColorSpace::MakeSRGB());
  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  SkImageInfo output;
  proxy->EchoImageInfo(input, &output);
  EXPECT_EQ(input, output);
}

TEST_F(StructTraitsTest, Bitmap) {
  SkBitmap input;
  input.allocN32Pixels(10, 5);
  input.eraseColor(SK_ColorYELLOW);
  input.erase(SK_ColorTRANSPARENT, SkIRect::MakeXYWH(0, 1, 2, 3));
  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  SkBitmap output;
  proxy->EchoBitmap(input, &output);
  EXPECT_EQ(input.colorType(), output.colorType());
  EXPECT_EQ(input.alphaType(), output.alphaType());
  EXPECT_EQ(colorspace_srgb_gamma(input.colorSpace()),
            colorspace_srgb_gamma(output.colorSpace()));
  EXPECT_EQ(input.width(), output.width());
  EXPECT_EQ(input.height(), output.height());
  EXPECT_EQ(input.rowBytes(), output.rowBytes());
  EXPECT_TRUE(gfx::BitmapsAreEqual(input, output));
}

TEST_F(StructTraitsTest, BitmapWithExtraRowBytes) {
  SkBitmap input;
  // Ensure traits work with bitmaps containing additional bytes between rows.
  SkImageInfo info = SkImageInfo::MakeN32(8, 5, kPremul_SkAlphaType);
  // Any extra bytes on each row must be a multiple of the row's pixel size to
  // keep every row's pixels aligned.
  size_t extra = info.bytesPerPixel();
  input.allocPixels(info, info.minRowBytes() + extra);
  input.eraseColor(SK_ColorRED);
  input.erase(SK_ColorTRANSPARENT, SkIRect::MakeXYWH(0, 1, 2, 3));
  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  SkBitmap output;
  proxy->EchoBitmap(input, &output);
  EXPECT_EQ(input.colorType(), output.colorType());
  EXPECT_EQ(input.alphaType(), output.alphaType());
  EXPECT_EQ(colorspace_srgb_gamma(input.colorSpace()),
            colorspace_srgb_gamma(output.colorSpace()));
  EXPECT_EQ(input.width(), output.width());
  EXPECT_EQ(input.height(), output.height());
  EXPECT_EQ(input.rowBytes(), output.rowBytes());
  EXPECT_TRUE(gfx::BitmapsAreEqual(input, output));
}

TEST_F(StructTraitsTest, BlurImageFilterTileMode) {
  SkBlurImageFilter::TileMode input(SkBlurImageFilter::kClamp_TileMode);
  mojom::TraitsTestServicePtr proxy = GetTraitsTestProxy();
  SkBlurImageFilter::TileMode output;
  proxy->EchoBlurImageFilterTileMode(input, &output);
  EXPECT_EQ(input, output);
}

}  // namespace skia

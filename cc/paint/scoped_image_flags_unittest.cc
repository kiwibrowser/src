// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/scoped_image_flags.h"

#include "base/bind.h"
#include "base/callback.h"
#include "cc/test/skia_common.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {
class MockImageProvider : public ImageProvider {
 public:
  MockImageProvider() = default;
  ~MockImageProvider() override { EXPECT_EQ(ref_count_, 0); }

  ScopedDecodedDrawImage GetDecodedDrawImage(
      const DrawImage& draw_image) override {
    ref_count_++;

    SkBitmap bitmap;
    bitmap.allocN32Pixels(10, 10);
    sk_sp<SkImage> image = SkImage::MakeFromBitmap(bitmap);

    return ScopedDecodedDrawImage(
        DecodedDrawImage(image, SkSize::MakeEmpty(), SkSize::Make(1.0f, 1.0f),
                         draw_image.filter_quality()),
        base::BindOnce(&MockImageProvider::UnrefImage, base::Unretained(this)));
  }

  void UnrefImage(DecodedDrawImage decoded_image) {
    ref_count_--;
    CHECK_GE(ref_count_, 0);
  }

  int ref_count() const { return ref_count_; }

 private:
  int ref_count_ = 0;
};
}  // namespace

TEST(ScopedImageFlagsTest, KeepsDecodesAlive) {
  auto record = sk_make_sp<PaintOpBuffer>();
  record->push<DrawImageOp>(CreateDiscardablePaintImage(gfx::Size(10, 10)), 0.f,
                            0.f, nullptr);
  record->push<DrawImageOp>(CreateDiscardablePaintImage(gfx::Size(10, 10)), 0.f,
                            0.f, nullptr);
  record->push<DrawImageOp>(CreateDiscardablePaintImage(gfx::Size(10, 10)), 0.f,
                            0.f, nullptr);
  auto record_shader = PaintShader::MakePaintRecord(
      record, SkRect::MakeWH(100, 100), SkShader::TileMode::kClamp_TileMode,
      SkShader::TileMode::kClamp_TileMode, &SkMatrix::I());

  MockImageProvider provider;
  PaintFlags flags;
  flags.setShader(record_shader);
  {
    ScopedImageFlags scoped_flags(&provider, flags, SkMatrix::I());
    ASSERT_TRUE(scoped_flags.decoded_flags());
    SkPaint paint = scoped_flags.decoded_flags()->ToSkPaint();
    ASSERT_TRUE(paint.getShader());
    EXPECT_EQ(provider.ref_count(), 3);
  }
  EXPECT_EQ(provider.ref_count(), 0);
}

}  // namespace cc

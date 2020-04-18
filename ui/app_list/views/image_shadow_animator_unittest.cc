// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/image_shadow_animator.h"

#include "base/macros.h"
#include "ui/views/test/views_test_base.h"

namespace {

bool ShadowsEqual(const gfx::ShadowValue& a, const gfx::ShadowValue& b) {
  return a.blur() == b.blur() && a.color() == b.color() &&
         a.offset() == b.offset();
}

}  // namespace

namespace app_list {
namespace test {

class ImageShadowAnimatorTest : public views::ViewsTestBase,
                                public ImageShadowAnimator::Delegate {
 public:
  ImageShadowAnimatorTest()
      : shadow_animator_(this), animation_updated_(false) {}
  ~ImageShadowAnimatorTest() override {}

  bool animation_updated() { return animation_updated_; }
  void reset_animation_updated() { animation_updated_ = false; }
  ImageShadowAnimator* shadow_animator() { return &shadow_animator_; }

  // ImageShadowAnimator::Delegate overrides:
  void ImageShadowAnimationProgressed(ImageShadowAnimator* animator) override {
    animation_updated_ = true;
  }

  void UpdateShadowImageForProgress(double progress) {
    shadow_animator_.UpdateShadowImageForProgress(progress);
  }

  gfx::ShadowValues GetShadowValuesForProgress(double progress) {
    return shadow_animator_.GetShadowValuesForProgress(progress);
  }

 private:
  ImageShadowAnimator shadow_animator_;
  bool animation_updated_;

  DISALLOW_COPY_AND_ASSIGN(ImageShadowAnimatorTest);
};

TEST_F(ImageShadowAnimatorTest, TweenShadow) {
  gfx::ShadowValues start;
  start.push_back(gfx::ShadowValue(gfx::Vector2d(0, 1), 2,
                                   SkColorSetA(SK_ColorBLACK, 0xA0)));
  start.push_back(gfx::ShadowValue(gfx::Vector2d(0, 2), 4,
                                   SkColorSetA(SK_ColorBLACK, 0x80)));

  gfx::ShadowValues end;
  end.push_back(gfx::ShadowValue(gfx::Vector2d(-2, 3), 4,
                                 SkColorSetA(SK_ColorBLACK, 0xC0)));
  end.push_back(gfx::ShadowValue(gfx::Vector2d(4, 4), 8,
                                 SkColorSetA(SK_ColorBLACK, 0x60)));

  shadow_animator()->SetStartAndEndShadows(start, end);

  gfx::ShadowValues result_shadows = GetShadowValuesForProgress(0.5);

  // Although the blur value of this shadow should be 3, it is snapped to an
  // even value to prevent offset issues in clients.
  EXPECT_TRUE(ShadowsEqual(gfx::ShadowValue(gfx::Vector2d(-1, 2), 4,
                                            SkColorSetA(SK_ColorBLACK, 0xB0)),
                           result_shadows[0]));

  EXPECT_TRUE(ShadowsEqual(gfx::ShadowValue(gfx::Vector2d(2, 3), 6,
                                            SkColorSetA(SK_ColorBLACK, 0x70)),
                           result_shadows[1]));
}

TEST_F(ImageShadowAnimatorTest, ImageSize) {
  gfx::ShadowValues start;
  start.push_back(gfx::ShadowValue(gfx::Vector2d(0, 2), 2, 0xA0000000));

  gfx::ShadowValues end;
  end.push_back(gfx::ShadowValue(gfx::Vector2d(0, 8), 8, 0x60000000));

  shadow_animator()->SetStartAndEndShadows(start, end);

  EXPECT_FALSE(animation_updated());

  SkBitmap bitmap;
  bitmap.allocN32Pixels(8, 8);
  bitmap.eraseColor(SK_ColorBLUE);
  shadow_animator()->SetOriginalImage(
      gfx::ImageSkia::CreateFrom1xBitmap(bitmap));
  // Setting the image should notify the delegate.
  EXPECT_TRUE(animation_updated());

  // Check the shadowed image grows as it animates, due to the increasing blur
  // and vertical offset.
  EXPECT_EQ(gfx::Size(10, 11), shadow_animator()->shadow_image().size());
  UpdateShadowImageForProgress(0.5);
  EXPECT_EQ(gfx::Size(14, 16), shadow_animator()->shadow_image().size());
  UpdateShadowImageForProgress(1);
  EXPECT_EQ(gfx::Size(16, 20), shadow_animator()->shadow_image().size());
}

}  // namespace test
}  // namespace app_list

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/bind.h"
#include "cc/paint/paint_image_builder.h"
#include "cc/test/skia_common.h"
#include "cc/tiles/decoded_image_tracker.h"
#include "cc/tiles/image_controller.h"
#include "cc/tiles/software_image_decode_cache.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {

class TestImageController : public ImageController {
 public:
  TestImageController() : ImageController(nullptr, nullptr) {}

  void UnlockImageDecode(ImageDecodeRequestId id) override {
    auto it = std::find_if(
        locked_ids_.begin(), locked_ids_.end(),
        [id](const std::pair<const ImageDecodeRequestId,
                             SoftwareImageDecodeCache::CacheKey>& item) {
          return item.first == id;
        });
    ASSERT_FALSE(it == locked_ids_.end());
    locked_ids_.erase(it);
  }

  ImageDecodeRequestId QueueImageDecode(
      const DrawImage& image,
      const ImageDecodedCallback& callback) override {
    auto id = next_id_++;
    locked_ids_.insert(
        std::make_pair(id, SoftwareImageDecodeCache::CacheKey::FromDrawImage(
                               image, kRGBA_8888_SkColorType)));
    callback.Run(id, ImageDecodeResult::SUCCESS);
    return id;
  }

  bool IsDrawImageLocked(const DrawImage& image) {
    SoftwareImageDecodeCache::CacheKey key =
        SoftwareImageDecodeCache::CacheKey::FromDrawImage(
            image, kRGBA_8888_SkColorType);
    return std::find_if(
               locked_ids_.begin(), locked_ids_.end(),
               [&key](
                   const std::pair<const ImageDecodeRequestId,
                                   SoftwareImageDecodeCache::CacheKey>& item) {
                 return item.second == key;
               }) != locked_ids_.end();
  }

  size_t num_locked_images() { return locked_ids_.size(); }

 private:
  ImageDecodeRequestId next_id_ = 1;
  std::unordered_map<ImageDecodeRequestId, SoftwareImageDecodeCache::CacheKey>
      locked_ids_;
};

class DecodedImageTrackerTest : public testing::Test {
 public:
  DecodedImageTrackerTest() : decoded_image_tracker_(&image_controller_) {}

  TestImageController* image_controller() { return &image_controller_; }
  DecodedImageTracker* decoded_image_tracker() {
    return &decoded_image_tracker_;
  }

 private:
  TestImageController image_controller_;
  DecodedImageTracker decoded_image_tracker_;
};

TEST_F(DecodedImageTrackerTest, QueueImageLocksImages) {
  bool locked = false;
  decoded_image_tracker()->QueueImageDecode(
      CreateDiscardablePaintImage(gfx::Size(1, 1)), gfx::ColorSpace(),
      base::Bind([](bool* locked, bool success) { *locked = true; },
                 base::Unretained(&locked)));
  EXPECT_TRUE(locked);
  EXPECT_EQ(1u, image_controller()->num_locked_images());
}

TEST_F(DecodedImageTrackerTest, NotifyFrameFinishedUnlocksImages) {
  bool locked = false;
  gfx::ColorSpace decoded_color_space(
      gfx::ColorSpace::PrimaryID::XYZ_D50,
      gfx::ColorSpace::TransferID::IEC61966_2_1);
  gfx::ColorSpace srgb_color_space = gfx::ColorSpace::CreateSRGB();
  auto paint_image = CreateDiscardablePaintImage(gfx::Size(1, 1));
  decoded_image_tracker()->QueueImageDecode(
      paint_image, decoded_color_space,
      base::Bind([](bool* locked, bool success) { *locked = true; },
                 base::Unretained(&locked)));
  EXPECT_TRUE(locked);
  EXPECT_EQ(1u, image_controller()->num_locked_images());

  decoded_image_tracker()->NotifyFrameFinished();
  EXPECT_EQ(1u, image_controller()->num_locked_images());

  // Check that the decoded color space images are locked, but if the color
  // space differs then that image is not locked. Note that we use the high
  // filter quality here, since it shouldn't matter and the checks should
  // succeed anyway.
  DrawImage locked_draw_image(paint_image, SkIRect::MakeWH(1, 1),
                              kHigh_SkFilterQuality, SkMatrix::I(),
                              paint_image.frame_index(), decoded_color_space);
  EXPECT_TRUE(image_controller()->IsDrawImageLocked(locked_draw_image));
  DrawImage srgb_draw_image(paint_image, SkIRect::MakeWH(1, 1),
                            kHigh_SkFilterQuality, SkMatrix::I(),
                            paint_image.frame_index(), srgb_color_space);
  EXPECT_FALSE(image_controller()->IsDrawImageLocked(srgb_draw_image));

  locked = false;
  decoded_image_tracker()->QueueImageDecode(
      CreateDiscardablePaintImage(gfx::Size(1, 1)), decoded_color_space,
      base::Bind([](bool* locked, bool success) { *locked = true; },
                 base::Unretained(&locked)));
  EXPECT_TRUE(locked);
  EXPECT_EQ(2u, image_controller()->num_locked_images());

  decoded_image_tracker()->NotifyFrameFinished();
  EXPECT_EQ(1u, image_controller()->num_locked_images());

  decoded_image_tracker()->NotifyFrameFinished();
  EXPECT_EQ(0u, image_controller()->num_locked_images());
}

}  // namespace cc

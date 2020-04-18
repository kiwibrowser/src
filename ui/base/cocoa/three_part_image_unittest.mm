// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/cocoa/three_part_image.h"

#include <memory>

#include "testing/gtest_mac.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkRect.h"
#include "ui/base/resource/resource_bundle.h"
#import "ui/base/test/cocoa_helper.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

namespace ui {
namespace test {

TEST(ThreePartImageTest, GetRects) {
  const int kHeight = 11;
  const int kLeftWidth = 3;
  const int kMiddleWidth = 5;
  const int kRightWidth = 7;
  base::scoped_nsobject<NSImage> leftImage(
      gfx::test::CreateImage(kLeftWidth, kHeight).CopyNSImage());
  base::scoped_nsobject<NSImage> middleImage(
      gfx::test::CreateImage(kMiddleWidth, kHeight).CopyNSImage());
  base::scoped_nsobject<NSImage> rightImage(
      gfx::test::CreateImage(kRightWidth, kHeight).CopyNSImage());
  ThreePartImage image(leftImage, middleImage, rightImage);
  NSRect bounds =
      NSMakeRect(0, 0, kLeftWidth + kMiddleWidth + kRightWidth, kHeight);
  EXPECT_NSRECT_EQ(NSMakeRect(0, 0, kLeftWidth, kHeight),
                   image.GetLeftRect(bounds));
  EXPECT_NSRECT_EQ(NSMakeRect(kLeftWidth, 0, kMiddleWidth, kHeight),
                   image.GetMiddleRect(bounds));
  EXPECT_NSRECT_EQ(
      NSMakeRect(kLeftWidth + kMiddleWidth, 0, kRightWidth, kHeight),
      image.GetRightRect(bounds));

  ThreePartImage image2(leftImage, nullptr, rightImage);
  EXPECT_NSRECT_EQ(NSMakeRect(0, 0, kLeftWidth, kHeight),
                   image.GetLeftRect(bounds));
  EXPECT_NSRECT_EQ(NSMakeRect(kLeftWidth, 0, kMiddleWidth, kHeight),
                   image.GetMiddleRect(bounds));
  EXPECT_NSRECT_EQ(
      NSMakeRect(kLeftWidth + kMiddleWidth, 0, kRightWidth, kHeight),
      image.GetRightRect(bounds));
}

TEST(ThreePartImageTest, HitTest) {
  // Create a bitmap with transparent top and bottom.
  const int size = 128;
  const int corner_size = 8;
  SkBitmap bitmap = gfx::test::CreateBitmap(size, size);
  // Clear top and bottom.
  bitmap.erase(SK_ColorTRANSPARENT, SkIRect::MakeXYWH(0, 0, size, corner_size));
  bitmap.erase(SK_ColorTRANSPARENT,
               SkIRect::MakeXYWH(0, size - corner_size, size, corner_size));
  gfx::Image part_image = gfx::Image::CreateFrom1xBitmap(bitmap);

  // Create a three-part image.
  base::scoped_nsobject<NSImage> ns_image(part_image.CopyNSImage());
  ThreePartImage image(ns_image, nullptr, ns_image);
  NSRect bounds = NSMakeRect(0, 0, 4 * size, size);

  // The middle of the left and right parts are hits.
  EXPECT_TRUE(image.HitTest(NSMakePoint(size / 2, size / 2), bounds));
  EXPECT_TRUE(image.HitTest(NSMakePoint(7 * size / 2, size / 2), bounds));

  // No middle image means the middle rect is a hit.
  EXPECT_TRUE(image.HitTest(NSMakePoint(2 * size, size / 2), bounds));

  // The corners are transparent.
  EXPECT_FALSE(image.HitTest(NSMakePoint(0, 0), bounds));
  EXPECT_FALSE(image.HitTest(NSMakePoint(0, size - 1), bounds));
  EXPECT_FALSE(image.HitTest(NSMakePoint(4 * size - 1, 0), bounds));
  EXPECT_FALSE(image.HitTest(NSMakePoint(4 * size - 1, size - 1), bounds));
}

}  // namespace test
}  // namespace ui

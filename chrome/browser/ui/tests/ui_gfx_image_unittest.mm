// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <AppKit/AppKit.h>

#import "base/mac/mac_util.h"
#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

namespace {

class UiGfxImageTest : public CocoaTest {
};

// http://crbug.com/247379
TEST_F(UiGfxImageTest, DISABLED_CheckColor) {
  // TODO(kbr): re-enable: http://crbug.com/222296
  return;

  gfx::Image image = gfx::Image::CreateFrom1xBitmap(
      gfx::test::CreateBitmap(25, 25));
  NSImage* ns_image = image.ToNSImage();
  [ns_image lockFocus];
  NSColor* color = NSReadPixel(NSMakePoint(10, 10));
  [ns_image unlockFocus];

  // SkBitmapToNSImage returns a bitmap in the calibrated color space (sRGB),
  // while NSReadPixel returns a color in the device color space. Convert back
  // to the calibrated color space before testing.
  color = [color colorUsingColorSpaceName:NSCalibratedRGBColorSpace];

  CGFloat components[4] = { 0 };
  [color getComponents:components];

  EXPECT_LT(components[0], 0.05);
  EXPECT_GT(components[1], 0.95);
  EXPECT_LT(components[2], 0.05);
  EXPECT_GT(components[3], 0.95);
}

TEST_F(UiGfxImageTest, ImageView) {
  base::scoped_nsobject<NSImageView> image_view(
      [[NSImageView alloc] initWithFrame:NSMakeRect(10, 10, 25, 25)]);
  [[test_window() contentView] addSubview:image_view];
  [test_window() orderFront:nil];

  gfx::Image image = gfx::Image::CreateFrom1xBitmap(
      gfx::test::CreateBitmap(25, 25));
  [image_view setImage:image.ToNSImage()];
}

}  // namespace

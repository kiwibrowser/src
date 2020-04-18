// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/location_bar/image_decoration.h"

#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class ImageDecorationTest : public CocoaTest {
 public:
  ImageDecoration decoration_;
};

TEST_F(ImageDecorationTest, SetGetImage) {
  EXPECT_FALSE(decoration_.GetImage());

  const NSSize kImageSize = NSMakeSize(20.0, 20.0);
  base::scoped_nsobject<NSImage> image(
      [[NSImage alloc] initWithSize:kImageSize]);

  decoration_.SetImage(image);
  EXPECT_EQ(decoration_.GetImage(), image);

  decoration_.SetImage(nil);
  EXPECT_FALSE(decoration_.GetImage());
}

TEST_F(ImageDecorationTest, GetWidthForSpace) {
  const CGFloat kWide = 100.0;
  const CGFloat kNarrow = 10.0;
  const CGFloat kImageHorizontalPadding = 10.0;

  // Decoration with no image is omitted.
  EXPECT_EQ(decoration_.GetWidthForSpace(kWide),
            LocationBarDecoration::kOmittedWidth);

  const NSSize kImageSize = NSMakeSize(20.0, 20.0);
  base::scoped_nsobject<NSImage> image(
      [[NSImage alloc] initWithSize:kImageSize]);

  // Decoration takes up the space of the image and the horizontal padding.
  decoration_.SetImage(image);
  EXPECT_EQ(decoration_.GetWidthForSpace(kWide),
            kImageSize.width + kImageHorizontalPadding);

  // If the image doesn't fit, decoration is omitted.
  EXPECT_EQ(decoration_.GetWidthForSpace(kNarrow),
            LocationBarDecoration::kOmittedWidth);
}

// TODO(shess): It would be nice to test mouse clicks and dragging,
// but those are hard because they require a real |owner|.

}  // namespace

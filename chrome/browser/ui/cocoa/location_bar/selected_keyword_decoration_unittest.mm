// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/strings/utf_string_conversions.h"
#import "chrome/browser/ui/cocoa/location_bar/selected_keyword_decoration.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "components/omnibox/browser/vector_icons.h"
#include "components/vector_icons/vector_icons.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

namespace {

// A wide width which should fit everything.
const CGFloat kWidth(300.0);

// A narrow width for tests which test things that don't fit.
const CGFloat kNarrowWidth(5.0);

}  // namespace

class SelectedKeywordDecorationTest : public CocoaTest {
 public:
  SelectedKeywordDecorationTest() {
    // With Material Design vector images the image isn't set at creation time
    // but later by the LocationBar. The unit tests fail with a nil image, so
    // initialize it now.
    const int kDefaultIconSize = 16;
    decoration_.SetImage(NSImageFromImageSkia(gfx::CreateVectorIcon(
        vector_icons::kSearchIcon, kDefaultIconSize, SK_ColorBLACK)));
  }

  SelectedKeywordDecoration decoration_;
};

// Test that the cell correctly chooses the partial keyword if there's
// not enough room.
TEST_F(SelectedKeywordDecorationTest, UsesPartialKeywordIfNarrow) {

  const base::string16 kKeyword = base::ASCIIToUTF16("Engine");
  NSString* const kFullString = @"Search Engine";
  NSString* const kPartialString = @"Search En\u2026";

  decoration_.SetKeyword(kKeyword, false);

  // Wide width chooses the full string and image.
  const CGFloat all_width = decoration_.GetWidthForSpace(kWidth);
  EXPECT_TRUE(decoration_.image_);
  EXPECT_NSEQ(kFullString, decoration_.label_);

  // If not enough space to include the image, uses exactly the full
  // string.
  const CGFloat full_width = decoration_.GetWidthForSpace(all_width - 5.0);
  EXPECT_LT(full_width, all_width);
  EXPECT_FALSE(decoration_.image_);
  EXPECT_NSEQ(kFullString, decoration_.label_);

  // Narrow width chooses the partial string.
  const CGFloat partial_width = decoration_.GetWidthForSpace(kNarrowWidth);
  EXPECT_LT(partial_width, full_width);
  EXPECT_FALSE(decoration_.image_);
  EXPECT_NSEQ(kPartialString, decoration_.label_);

  // Narrow doesn't choose partial string if there is not one.
  decoration_.partial_string_.reset();
  decoration_.GetWidthForSpace(kNarrowWidth);
  EXPECT_FALSE(decoration_.image_);
  EXPECT_NSEQ(kFullString, decoration_.label_);
}

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_footer_item.h"

#import "ios/chrome/browser/ui/collection_view/cells/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using CollectionViewFooterItemTest = PlatformTest;

// Tests that the text label and the image are set properly after a call to
// |configureCell:|.
TEST_F(CollectionViewFooterItemTest, ConfigureCell) {
  CollectionViewFooterItem* item =
      [[CollectionViewFooterItem alloc] initWithType:0];
  NSString* text = @"Test Footer";
  item.text = text;

  UIImage* image = CollectionViewTestImage();
  item.image = image;

  CollectionViewFooterCell* cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[CollectionViewFooterCell class]]);
  EXPECT_FALSE(cell.textLabel.text);
  EXPECT_FALSE(cell.imageView.image);

  [item configureCell:cell];
  EXPECT_NSEQ(text, cell.textLabel.text);
  EXPECT_NSEQ(image, cell.imageView.image);
}

}  // namespace

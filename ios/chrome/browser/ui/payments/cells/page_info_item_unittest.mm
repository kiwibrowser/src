// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/cells/page_info_item.h"

#import "ios/chrome/browser/ui/collection_view/cells/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using PaymentRequestPageInfoItemTest = PlatformTest;

// Tests that the labels and image are set properly after a call to
// |configureCell:|.
TEST_F(PaymentRequestPageInfoItemTest, TextLabels) {
  PageInfoItem* item = [[PageInfoItem alloc] init];

  UIImage* pageFavicon = CollectionViewTestImage();
  NSString* pageTitle = @"The Greatest Website Ever";
  NSString* pageHost = @"http://localhost";
  NSString* pageHostSecure = @"https://www.greatest.example.com";

  item.pageFavicon = pageFavicon;
  item.pageTitle = pageTitle;
  item.pageHost = pageHost;
  item.connectionSecure = false;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[PageInfoCell class]]);

  PageInfoCell* pageInfoCell = cell;
  EXPECT_FALSE(pageInfoCell.pageTitleLabel.text);
  EXPECT_FALSE(pageInfoCell.pageHostLabel.text);
  EXPECT_FALSE(pageInfoCell.pageFaviconView.image);
  EXPECT_FALSE(pageInfoCell.pageLockIndicatorView.image);

  [item configureCell:pageInfoCell];
  EXPECT_NSEQ(pageTitle, pageInfoCell.pageTitleLabel.text);
  EXPECT_NSEQ(pageHost, pageInfoCell.pageHostLabel.text);
  EXPECT_NSEQ(pageFavicon, pageInfoCell.pageFaviconView.image);
  EXPECT_FALSE(pageInfoCell.pageLockIndicatorView.image);

  item.pageHost = pageHostSecure;
  item.connectionSecure = true;

  id cell2 = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell2 isMemberOfClass:[PageInfoCell class]]);

  PageInfoCell* pageInfoCell2 = cell2;
  EXPECT_FALSE(pageInfoCell2.pageTitleLabel.text);
  EXPECT_FALSE(pageInfoCell2.pageHostLabel.text);
  EXPECT_FALSE(pageInfoCell2.pageFaviconView.image);
  EXPECT_FALSE(pageInfoCell2.pageLockIndicatorView.image);

  [item configureCell:pageInfoCell2];
  EXPECT_NSEQ(pageTitle, pageInfoCell2.pageTitleLabel.text);
  EXPECT_NSEQ(pageHostSecure, pageInfoCell2.pageHostLabel.text);
  EXPECT_NSEQ(pageFavicon, pageInfoCell2.pageFaviconView.image);
  EXPECT_TRUE(pageInfoCell2.pageLockIndicatorView.image);
}

}  // namespace

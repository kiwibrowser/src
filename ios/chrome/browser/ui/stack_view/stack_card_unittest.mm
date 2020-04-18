// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/stack_view/card_view.h"
#import "ios/chrome/browser/ui/stack_view/stack_card.h"
#import "ios/chrome/browser/ui/ui_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Mocked-out CardView object
@interface MockCardView : UIView
@end

@implementation MockCardView
- (void)setIsActiveTab:(BOOL)isActiveTab {
}
@end

// Returns mocked-out CardView objects for every request.
@interface MockCardViewProvider : NSObject<StackCardViewProvider>
@end

@implementation MockCardViewProvider
- (CardView*)cardViewWithFrame:(CGRect)frame forStackCard:(StackCard*)card {
  return static_cast<CardView*>([[MockCardView alloc] initWithFrame:frame]);
}
@end

#pragma mark -

namespace {

class StackCardTest : public PlatformTest {
 protected:
  StackCardTest() { view_provider_ = [[MockCardViewProvider alloc] init]; }

  MockCardViewProvider* view_provider_;
};

TEST_F(StackCardTest, LazyCreation) {
  StackCard* card = [[StackCard alloc] initWithViewProvider:view_provider_];
  // Set attributes before asking for the view.
  LayoutRect layout = LayoutRectMake(10, 300, 20, 55, 98);
  CGRect frame = LayoutRectGetRect(layout);
  [card setLayout:layout];
  // Ensure the view hasn't been created yet.
  EXPECT_FALSE([card viewIsLive]);
  // Make sure that the view actually has those attributes.
  UIView* view = [card view];
  EXPECT_FLOAT_EQ(frame.size.width, view.frame.size.width);
  EXPECT_FLOAT_EQ(frame.size.height, view.frame.size.height);
  EXPECT_FLOAT_EQ(frame.origin.x, view.frame.origin.x);
  EXPECT_FLOAT_EQ(frame.origin.y, view.frame.origin.y);
}

TEST_F(StackCardTest, LiveViewUpdating) {
  StackCard* card = [[StackCard alloc] initWithViewProvider:view_provider_];
  // Get the view, then set attributes.
  UIView* view = [card view];
  LayoutRect layout = LayoutRectMake(10, 300, 20, 55, 98);
  CGRect frame = LayoutRectGetRect(layout);
  [card setLayout:layout];
  // Make sure that the view actually has those attributes.
  EXPECT_FLOAT_EQ(frame.size.width, view.frame.size.width);
  EXPECT_FLOAT_EQ(frame.size.height, view.frame.size.height);
  EXPECT_FLOAT_EQ(frame.origin.x, view.frame.origin.x);
  EXPECT_FLOAT_EQ(frame.origin.y, view.frame.origin.y);
}

TEST_F(StackCardTest, BoundsUpdatePreservesCenter) {
  StackCard* card = [[StackCard alloc] initWithViewProvider:view_provider_];
  LayoutRect layout = LayoutRectMake(0, 300, 0, 40, 100);
  CGRect frame = LayoutRectGetRect(layout);
  [card setLayout:layout];
  // Changing the bounds should preserve the center (as with UIView).
  [card setSize:CGSizeMake(10, 10)];
  CGRect newFrame = LayoutRectGetRect([card layout]);
  EXPECT_FLOAT_EQ(CGRectGetMidX(frame), CGRectGetMidX(newFrame));
  EXPECT_FLOAT_EQ(CGRectGetMidY(frame), CGRectGetMidY(newFrame));
}

TEST_F(StackCardTest, PixelAlignmentOfViewFrameAfterLiveUpdate) {
  StackCard* card = [[StackCard alloc] initWithViewProvider:view_provider_];
  // Get the view, then set attributes.
  UIView* view = [card view];
  const LayoutRectPosition kPosition = LayoutRectPositionMake(10.3, 20.4);
  const CGSize kSize = CGSizeMake(55, 98);
  const CGFloat kBoundingWidth = 300;
  const LayoutRect kLayout =
      LayoutRectMake(kPosition.leading, kBoundingWidth, kPosition.originY,
                     kSize.width, kSize.height);
  [card setLayout:kLayout];
  EXPECT_FLOAT_EQ(kSize.width, view.frame.size.width);
  EXPECT_FLOAT_EQ(kSize.height, view.frame.size.height);
  EXPECT_FLOAT_EQ(kPosition.leading, [card layout].position.leading);
  EXPECT_FLOAT_EQ(kPosition.originY, [card layout].position.originY);
  // The view's origin should be pixel-aligned.
  const CGPoint kPixelAlignedOrigin =
      AlignRectOriginAndSizeToPixels(LayoutRectGetRect(kLayout)).origin;
  EXPECT_FLOAT_EQ(kPixelAlignedOrigin.x, view.frame.origin.x);
  EXPECT_FLOAT_EQ(kPixelAlignedOrigin.y, view.frame.origin.y);
}

TEST_F(StackCardTest, ViewFrameSynchronization) {
  StackCard* card = [[StackCard alloc] initWithViewProvider:view_provider_];
  // Get the view, then set attributes.
  UIView* view = [card view];
  const LayoutRect kFirstLayout = LayoutRectMake(10, 300, 20, 55, 98);
  CGRect firstFrame = LayoutRectGetRect(kFirstLayout);
  [card setLayout:kFirstLayout];
  // Make sure that the view actually has those attributes.
  EXPECT_FLOAT_EQ(firstFrame.size.width, view.frame.size.width);
  EXPECT_FLOAT_EQ(firstFrame.size.height, view.frame.size.height);
  EXPECT_FLOAT_EQ(firstFrame.origin.x, view.frame.origin.x);
  EXPECT_FLOAT_EQ(firstFrame.origin.y, view.frame.origin.y);
  [card setSynchronizeView:NO];
  const LayoutRect kSecondLayout = LayoutRectMake(5, 300, 10, 40, 75);
  CGRect secondFrame = LayoutRectGetRect(kSecondLayout);
  [card setLayout:kSecondLayout];
  // Card should have the new attributes...
  CGRect card_frame = LayoutRectGetRect([card layout]);
  EXPECT_FLOAT_EQ(secondFrame.size.width, card_frame.size.width);
  EXPECT_FLOAT_EQ(secondFrame.size.height, card_frame.size.height);
  EXPECT_FLOAT_EQ(secondFrame.origin.x, card_frame.origin.x);
  EXPECT_FLOAT_EQ(secondFrame.origin.y, card_frame.origin.y);
  // ... but view should still have the old attributes.
  EXPECT_FLOAT_EQ(firstFrame.size.width, view.frame.size.width);
  EXPECT_FLOAT_EQ(firstFrame.size.height, view.frame.size.height);
  EXPECT_FLOAT_EQ(firstFrame.origin.x, view.frame.origin.x);
  EXPECT_FLOAT_EQ(firstFrame.origin.y, view.frame.origin.y);
  [card setSynchronizeView:YES];
  // View should immediately pick up the new attributes.
  EXPECT_FLOAT_EQ(secondFrame.size.width, view.frame.size.width);
  EXPECT_FLOAT_EQ(secondFrame.size.height, view.frame.size.height);
  EXPECT_FLOAT_EQ(secondFrame.origin.x, view.frame.origin.x);
  EXPECT_FLOAT_EQ(secondFrame.origin.y, view.frame.origin.y);
}

TEST_F(StackCardTest, ViewLayoutSynchronization) {
  StackCard* card = [[StackCard alloc] initWithViewProvider:view_provider_];
  // Get the view, then set attributes.
  UIView* view = [card view];
  const LayoutRect kFirstLayout = LayoutRectMake(30, 300, 40, 200, 100);
  const CGRect kFirstFrame = LayoutRectGetRect(kFirstLayout);
  [card setLayout:kFirstLayout];
  // Make sure that the view actually has those attributes.
  EXPECT_FLOAT_EQ(kFirstFrame.size.width, view.bounds.size.width);
  EXPECT_FLOAT_EQ(kFirstFrame.size.height, view.bounds.size.height);
  EXPECT_FLOAT_EQ(CGRectGetMidX(kFirstFrame), view.center.x);
  EXPECT_FLOAT_EQ(CGRectGetMidY(kFirstFrame), view.center.y);
  [card setSynchronizeView:NO];
  const LayoutRect kSecondLayout = LayoutRectMake(20, 300, 10, 40, 50);
  const CGRect kSecondFrame = LayoutRectGetRect(kSecondLayout);
  [card setLayout:kSecondLayout];
  // Card should have the new attributes...
  EXPECT_FLOAT_EQ(kSecondLayout.position.leading,
                  [card layout].position.leading);
  EXPECT_FLOAT_EQ(kSecondLayout.position.originY,
                  [card layout].position.originY);
  EXPECT_FLOAT_EQ(kSecondLayout.size.width, [card layout].size.width);
  EXPECT_FLOAT_EQ(kSecondLayout.size.height, [card layout].size.height);
  // ... but view should still have the old attributes.
  EXPECT_FLOAT_EQ(kFirstFrame.size.width, view.bounds.size.width);
  EXPECT_FLOAT_EQ(kFirstFrame.size.height, view.bounds.size.height);
  EXPECT_FLOAT_EQ(CGRectGetMidX(kFirstFrame), view.center.x);
  EXPECT_FLOAT_EQ(CGRectGetMidY(kFirstFrame), view.center.y);
  [card setSynchronizeView:YES];
  // View should immediately pick up the new attributes.
  EXPECT_FLOAT_EQ(kSecondFrame.size.width, view.bounds.size.width);
  EXPECT_FLOAT_EQ(kSecondFrame.size.height, view.bounds.size.height);
  EXPECT_FLOAT_EQ(CGRectGetMidX(kSecondFrame), view.center.x);
  EXPECT_FLOAT_EQ(CGRectGetMidY(kSecondFrame), view.center.y);
}

TEST_F(StackCardTest, PixelAlignmentOfViewAfterSynchronization) {
  StackCard* card = [[StackCard alloc] initWithViewProvider:view_provider_];
  // Get the view, then set attributes.
  UIView* view = [card view];
  const CGFloat kBoundingWidth = 300;
  const LayoutRect kFirstLayout =
      LayoutRectMake(10, kBoundingWidth, 20, 55, 98);
  const CGRect kFirstFrame = LayoutRectGetRect(kFirstLayout);
  [card setLayout:kFirstLayout];
  // Make sure that the view actually has those attributes.
  EXPECT_FLOAT_EQ(kFirstFrame.size.width, view.frame.size.width);
  EXPECT_FLOAT_EQ(kFirstFrame.size.height, view.frame.size.height);
  EXPECT_FLOAT_EQ(kFirstFrame.origin.x, view.frame.origin.x);
  EXPECT_FLOAT_EQ(kFirstFrame.origin.y, view.frame.origin.y);
  [card setSynchronizeView:NO];
  const LayoutRectPosition kSecondPosition = LayoutRectPositionMake(8.72, 7.73);
  const CGSize kSecondSize = CGSizeMake(40, 75);
  const LayoutRect kSecondLayout = LayoutRectMake(
      kSecondPosition.leading, kBoundingWidth, kSecondPosition.originY,
      kSecondSize.width, kSecondSize.height);
  const CGRect kSecondFrame = LayoutRectGetRect(kSecondLayout);
  [card setLayout:kSecondLayout];
  // Card should have the new attributes...
  EXPECT_FLOAT_EQ(kSecondPosition.leading, [card layout].position.leading);
  EXPECT_FLOAT_EQ(kSecondPosition.originY, [card layout].position.originY);
  EXPECT_FLOAT_EQ(kSecondSize.width, [card layout].size.width);
  EXPECT_FLOAT_EQ(kSecondSize.height, [card layout].size.height);
  // ... but view should still have the old attributes.
  EXPECT_FLOAT_EQ(kFirstFrame.size.width, view.frame.size.width);
  EXPECT_FLOAT_EQ(kFirstFrame.size.height, view.frame.size.height);
  EXPECT_FLOAT_EQ(kFirstFrame.origin.x, view.frame.origin.x);
  EXPECT_FLOAT_EQ(kFirstFrame.origin.y, view.frame.origin.y);
  [card setSynchronizeView:YES];
  // View should immediately pick up the new attributes, with the origin
  // correctly pixel-aligned.
  EXPECT_FLOAT_EQ(kSecondFrame.size.width, view.frame.size.width);
  EXPECT_FLOAT_EQ(kSecondFrame.size.height, view.frame.size.height);
  const CGPoint kPixelAlignedOrigin =
      AlignRectOriginAndSizeToPixels(LayoutRectGetRect(kSecondLayout)).origin;
  EXPECT_FLOAT_EQ(kPixelAlignedOrigin.x, view.frame.origin.x);
  EXPECT_FLOAT_EQ(kPixelAlignedOrigin.y, view.frame.origin.y);
}

}  // namespace

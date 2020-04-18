// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <QuartzCore/QuartzCore.h>

#include "base/logging.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#include "ios/chrome/browser/ui/main/main_feature_flags.h"
#import "ios/chrome/browser/ui/stack_view/card_set.h"
#import "ios/chrome/browser/ui/stack_view/stack_card.h"
#import "ios/chrome/browser/ui/stack_view/stack_view_controller.h"
#import "ios/chrome/browser/ui/stack_view/stack_view_controller_private.h"
#include "ios/chrome/test/block_cleanup_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const CGFloat kViewportDimension = 200;

}  // namespace

#pragma mark -

@interface MockCardSet : NSObject {
 @private
  UIView* displayView_;
  CGSize cardSize_;
  CGFloat layoutAxisPosition_;
  BOOL initialConfigurationSet_;
}

// CardSet simulation
@property(nonatomic, strong, readwrite) UIView* displayView;
@property(nonatomic, assign, readwrite) CGSize cardSize;
@property(nonatomic, weak, readwrite) id<CardSetObserver> observer;
@property(nonatomic, assign, readwrite) BOOL keepOnlyVisibleCardViewsAlive;

- (void)configureLayoutParametersWithMargin:(CGFloat)margin;
- (void)setLayoutAxisPosition:(CGFloat)position
                   isVertical:(BOOL)layoutIsVertical;
- (void)clearGestureRecognizerTargetAndDelegateFromCards:(id)object;
- (NSArray*)cards;
- (CGFloat)maximumStackLength;
- (void)displayViewSizeWasChanged;
- (TabModel*)tabModel;
- (void)setTabModel:(TabModel*)tabModel;

// Testing helpers
- (BOOL)configured;
@property(nonatomic, assign, readonly) CGFloat layoutAxisPosition;
@end

@implementation MockCardSet

@synthesize displayView = displayView_;
@synthesize cardSize = cardSize_;
@synthesize observer = observer_;
@synthesize layoutAxisPosition = layoutAxisPosition_;
@synthesize keepOnlyVisibleCardViewsAlive = keepOnlyVisibleCardViewsAlive_;

- (BOOL)configured {
  return initialConfigurationSet_ && (layoutAxisPosition_ != 0);
}

- (void)configureLayoutParametersWithMargin:(CGFloat)margin {
  initialConfigurationSet_ = YES;
}

- (void)setLayoutAxisPosition:(CGFloat)position
                   isVertical:(BOOL)layoutIsVertical {
  layoutAxisPosition_ = position;
}

- (void)clearGestureRecognizerTargetAndDelegateFromCards:(id)object {
}

- (NSArray*)cards {
  // TODO(stuartmorgan): Return an actual set of cards to allow for more
  // interesting tests. For now this is just to satisfy the toolbar updating
  // code that shows the card count.
  return [NSArray array];
}

- (CGFloat)maximumStackLength {
  return 2.0 * kViewportDimension;
}

- (void)displayViewSizeWasChanged {
}

- (TabModel*)tabModel {
  return nil;
}

- (void)setTabModel:(TabModel*)tabModel {
}

@end

#pragma mark -

namespace {

class StackViewControllerTest : public BlockCleanupTest {
 protected:
  void SetUp() override {
    BlockCleanupTest::SetUp();

    main_card_set_ = [[MockCardSet alloc] init];
    otr_card_set_ = [[MockCardSet alloc] init];

    view_controller_ = [[StackViewController alloc]
               initWithMainCardSet:static_cast<CardSet*>(main_card_set_)
                        otrCardSet:static_cast<CardSet*>(otr_card_set_)
                     activeCardSet:static_cast<CardSet*>(main_card_set_)
        applicationCommandEndpoint:nil];
    // Resize the view and call VC lifecycle events
    CGRect frame = CGRectMake(0.0, 0.0, kViewportDimension, kViewportDimension);
    [view_controller_ view].frame = frame;

    // Simulate displaying the view.
    if (TabSwitcherPresentsBVCEnabled()) {
      [view_controller_ prepareForDisplayAtSize:frame.size];
    }
    [view_controller_ viewWillAppear:NO];
  }
  void TearDown() override {
    // The view controller uses a delayed selector call, so in the unittests
    // that causes the controller to be retained and outlive the test.
    [NSObject cancelPreviousPerformRequestsWithTarget:view_controller_];
    // And there are likely animations still running.
    for (UIView* view in [[view_controller_ scrollView] subviews]) {
      // Remove any animations on cards themselves.
      for (UIView* subview in [view subviews]) {
        [subview.layer removeAllAnimations];
      }
      [view.layer removeAllAnimations];
    }
    [[[view_controller_ scrollView] layer] removeAllAnimations];

    // Manually run teardown.
    [view_controller_ cleanUpViewsAndNotifications];

    BlockCleanupTest::TearDown();
  }

  // Checks that the display view of |card_set| is correctly set up:
  // - sized to match the size of the scroll view.
  // - origin set to 0 on the non-layout axis, and to the scroll view's content
  //   offset on the layout axis.
  // Also checks that the content size of the scroll view is configured
  // appropriately for this card set:
  // - greater than the maximum stack size along the layout axis to give enough
  //   space to fully scroll the cards to beginning/end without hitting the
  //   scroll view's boundaries.
  // - equal to the display view size along the non-layout axis.
  void ValidateDisplaySetupForCardSet(MockCardSet* card_set) {
    DCHECK(card_set == main_card_set_ || card_set == otr_card_set_);
    EXPECT_FLOAT_EQ([[card_set displayView] bounds].size.width,
                    [[view_controller_ scrollView] bounds].size.width);
    EXPECT_FLOAT_EQ([[card_set displayView] bounds].size.height,
                    [[view_controller_ scrollView] bounds].size.height);
    CGPoint display_view_origin = [[card_set displayView] frame].origin;
    BOOL is_portrait = UIInterfaceOrientationIsPortrait(
        [UIApplication sharedApplication].statusBarOrientation);
    CGFloat content_offset =
        is_portrait ? [[view_controller_ scrollView] contentOffset].y
                    : [[view_controller_ scrollView] contentOffset].x;
    if (is_portrait) {
      EXPECT_FLOAT_EQ(display_view_origin.x, 0);
      EXPECT_FLOAT_EQ(display_view_origin.y, content_offset);
      EXPECT_GT([[view_controller_ scrollView] contentSize].height,
                [card_set maximumStackLength]);
      EXPECT_FLOAT_EQ(card_set.displayView.bounds.size.width,
                      [[view_controller_ scrollView] contentSize].width);
    } else {
      EXPECT_FLOAT_EQ(display_view_origin.x, content_offset);
      EXPECT_FLOAT_EQ(display_view_origin.y, 0);
      EXPECT_GT([[view_controller_ scrollView] contentSize].width,
                [card_set maximumStackLength]);
      EXPECT_FLOAT_EQ(card_set.displayView.bounds.size.height,
                      [[view_controller_ scrollView] contentSize].height);
    }
  }

  StackViewController* view_controller_;
  MockCardSet* main_card_set_;
  MockCardSet* otr_card_set_;
};

TEST_F(StackViewControllerTest, BasicConfiguration) {
  // Ensure that the CardSet is configured and correctly laid out.
  EXPECT_TRUE([main_card_set_ configured]);
  EXPECT_TRUE([otr_card_set_ configured]);
  ValidateDisplaySetupForCardSet(main_card_set_);
  ValidateDisplaySetupForCardSet(otr_card_set_);
  EXPECT_GT([main_card_set_ cardSize].width, 0U);
  EXPECT_GT([main_card_set_ cardSize].height, 0U);

  // Incognito should always be right of (or below in landscape) the main set.
  EXPECT_GT([otr_card_set_ layoutAxisPosition],
            [main_card_set_ layoutAxisPosition]);
}

TEST_F(StackViewControllerTest, IncognitoHandling) {
  EXPECT_FALSE([view_controller_ isCurrentSetIncognito]);
  EXPECT_EQ(static_cast<CardSet*>(otr_card_set_),
            [view_controller_ inactiveCardSet]);
  [view_controller_ setActiveCardSet:[view_controller_ inactiveCardSet]];
  EXPECT_TRUE([view_controller_ isCurrentSetIncognito]);
  EXPECT_EQ(static_cast<CardSet*>(main_card_set_),
            [view_controller_ inactiveCardSet]);
  // Incognito should always be right of (or below in landscape) the main set.
  EXPECT_GT([otr_card_set_ layoutAxisPosition],
            [main_card_set_ layoutAxisPosition]);
}

TEST_F(StackViewControllerTest, ScrollViewContentOffsetRecentering) {
  BOOL is_portrait = UIInterfaceOrientationIsPortrait(
      [UIApplication sharedApplication].statusBarOrientation);
  CGFloat scroll_view_breadth =
      is_portrait ? [[view_controller_ scrollView] contentSize].width
                  : [[view_controller_ scrollView] contentSize].height;
  CGFloat content_size =
      is_portrait ? [[view_controller_ scrollView] contentSize].height
                  : [[view_controller_ scrollView] contentSize].width;

  // Test that recentering occurs after content offset gets set to lower
  // boundary.
  CGPoint newContentOffset = is_portrait ? CGPointMake(scroll_view_breadth, 0)
                                         : CGPointMake(0, scroll_view_breadth);
  [[view_controller_ scrollView] setContentOffset:newContentOffset];
  [view_controller_ recenterScrollViewIfNecessary];
  CGFloat content_offset =
      is_portrait ? [[view_controller_ scrollView] contentOffset].y
                  : [[view_controller_ scrollView] contentOffset].x;
  EXPECT_GT(content_offset, 0);
  EXPECT_LT(content_offset, content_size);
  ValidateDisplaySetupForCardSet(main_card_set_);
  ValidateDisplaySetupForCardSet(otr_card_set_);

  // Test that recentering occurs after content offset gets set to upper
  // boundary.
  newContentOffset = is_portrait
                         ? CGPointMake(scroll_view_breadth, content_size)
                         : CGPointMake(content_size, scroll_view_breadth);
  [[view_controller_ scrollView] setContentOffset:newContentOffset];
  [view_controller_ recenterScrollViewIfNecessary];
  content_offset = is_portrait
                       ? [[view_controller_ scrollView] contentOffset].y
                       : [[view_controller_ scrollView] contentOffset].x;
  EXPECT_GT(content_offset, 0);
  EXPECT_LT(content_offset, content_size);
  ValidateDisplaySetupForCardSet(main_card_set_);
  ValidateDisplaySetupForCardSet(otr_card_set_);
}

TEST_F(StackViewControllerTest, TabModelReset) {
  [view_controller_ setOtrTabModel:nil];
  ValidateDisplaySetupForCardSet(main_card_set_);
  ValidateDisplaySetupForCardSet(otr_card_set_);
}

}  // namespace

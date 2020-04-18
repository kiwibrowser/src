// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/side_swipe/card_side_swipe_view.h"

#include <cmath>

#include "base/ios/device_util.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/background_generator.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_panel_protocol.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/side_swipe/side_swipe_util.h"
#import "ios/chrome/browser/ui/side_swipe_gesture_recognizer.h"
#import "ios/chrome/browser/ui/toolbar/public/side_swipe_toolbar_snapshot_providing.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/chrome/browser/web/page_placeholder_tab_helper.h"
#include "ios/chrome/grit/ios_theme_resources.h"
#include "ios/web/public/features.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using base::UserMetricsAction;

namespace {
// Spacing between cards.
const CGFloat kCardHorizontalSpacing = 30;

// Portion of the screen an edge card can be dragged.
const CGFloat kEdgeCardDragPercentage = 0.35;

// Card animation times.
const NSTimeInterval kAnimationDuration = 0.15;

// Reduce size in -smallGreyImage by this factor.
const CGFloat kResizeFactor = 4;
}  // anonymous namespace

@interface SwipeView : UIView

@property(nonatomic, strong) UIImageView* topToolbarSnapshot;
@property(nonatomic, strong) UIImageView* bottomToolbarSnapshot;

@property CGFloat topMargin;
@property NSLayoutConstraint* toolbarTopConstraint;

@end

@implementation SwipeView {
  UIImageView* _image;
  // TODO(crbug.com/800266): Remove the shadow.
  UIImageView* _shadowView;
}

@synthesize topToolbarSnapshot = _topToolbarSnapshot;
@synthesize bottomToolbarSnapshot = _bottomToolbarSnapshot;
@synthesize topMargin = _topMargin;
@synthesize toolbarTopConstraint = _toolbarTopConstraint;

- (instancetype)initWithFrame:(CGRect)frame topMargin:(CGFloat)topMargin {
  self = [super initWithFrame:frame];
  if (self) {
    _topMargin = topMargin;

    _image = [[UIImageView alloc] initWithFrame:CGRectZero];
    [_image setClipsToBounds:YES];
    [_image setContentMode:UIViewContentModeScaleAspectFill];
    [self addSubview:_image];

    _topToolbarSnapshot = [[UIImageView alloc] initWithFrame:CGRectZero];
    [self addSubview:_topToolbarSnapshot];

    _bottomToolbarSnapshot = [[UIImageView alloc] initWithFrame:CGRectZero];
    [self addSubview:_bottomToolbarSnapshot];

    if (!IsUIRefreshPhase1Enabled()) {
      _shadowView = [[UIImageView alloc] initWithFrame:self.bounds];
      [_shadowView setImage:NativeImage(IDR_IOS_TOOLBAR_SHADOW)];
      [self addSubview:_shadowView];
    }

    // All subviews are as wide as the parent
    NSMutableArray* constraints = [NSMutableArray array];
    for (UIView* view in self.subviews) {
      [view setTranslatesAutoresizingMaskIntoConstraints:NO];
      [constraints addObject:[view.leadingAnchor
                                 constraintEqualToAnchor:self.leadingAnchor]];
      [constraints addObject:[view.trailingAnchor
                                 constraintEqualToAnchor:self.trailingAnchor]];
    }

    _toolbarTopConstraint = [[_topToolbarSnapshot topAnchor]
        constraintEqualToAnchor:self.topAnchor];

    if (!base::FeatureList::IsEnabled(
            web::features::kBrowserContainerFullscreen)) {
      _toolbarTopConstraint.constant = -StatusBarHeight();
    }

    [constraints addObjectsFromArray:@[
      [[_image topAnchor] constraintEqualToAnchor:self.topAnchor
                                         constant:topMargin],
      [[_image bottomAnchor] constraintEqualToAnchor:self.bottomAnchor],
      _toolbarTopConstraint,
      [_bottomToolbarSnapshot.bottomAnchor
          constraintEqualToAnchor:self.bottomAnchor],
    ]];

    [NSLayoutConstraint activateConstraints:constraints];

    if (!IsUIRefreshPhase1Enabled()) {
      [NSLayoutConstraint activateConstraints:@[
        [[_shadowView topAnchor] constraintEqualToAnchor:self.topAnchor
                                                constant:topMargin],
        [[_shadowView heightAnchor]
            constraintEqualToConstant:kNewTabPageShadowHeight],
      ]];
    }
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  [self updateImageBoundsAndZoom];
}

- (void)updateImageBoundsAndZoom {
  UIImage* image = [_image image];
  if (image) {
    CGSize imageSize = image.size;
    CGSize viewSize = [_image frame].size;
    CGFloat zoomRatio = std::max(viewSize.height / imageSize.height,
                                 viewSize.width / imageSize.width);
    [_image layer].contentsRect =
        CGRectMake(0.0, 0.0, viewSize.width / (zoomRatio * imageSize.width),
                   viewSize.height / (zoomRatio * imageSize.height));
  }
}

- (void)setImage:(UIImage*)image {
  [_image setImage:image];
  [self updateImageBoundsAndZoom];
}

- (void)setTopToolbarImage:(UIImage*)image isNewTabPage:(BOOL)isNewTabPage {
  [self.topToolbarSnapshot setImage:image];
  if (!base::FeatureList::IsEnabled(
          web::features::kBrowserContainerFullscreen)) {
    // Update constraints as StatusBarHeight changes depending on orientation.
    self.toolbarTopConstraint.constant = -StatusBarHeight();
  }
  [self.topToolbarSnapshot setNeedsLayout];
  [_shadowView setHidden:isNewTabPage];
}

- (void)setBottomToolbarImage:(UIImage*)image {
  [self.bottomToolbarSnapshot setImage:image];
  [self.bottomToolbarSnapshot setNeedsLayout];
}

@end

@interface CardSideSwipeView ()

// Pan touches ended or were cancelled.
- (void)finishPan;
// Is the current card is an edge card based on swipe direction.
- (BOOL)isEdgeSwipe;
// Initialize card based on model_ index.
- (void)setupCard:(SwipeView*)card withIndex:(NSInteger)index;
// Build a |kResizeFactor| sized greyscaled version of |image|.
- (UIImage*)smallGreyImage:(UIImage*)image;

@property(nonatomic, strong) NSLayoutConstraint* backgroundTopConstraint;
@end

@implementation CardSideSwipeView {
  // The direction of the swipe that initiated this horizontal view.
  UISwipeGestureRecognizerDirection _direction;

  // Card views currently displayed.
  SwipeView* _leftCard;
  SwipeView* _rightCard;

  // Most recent touch location.
  CGPoint currentPoint_;

  // Tab model.
  __weak TabModel* model_;

  // The image view containing the background image.
  UIImageView* backgroundView_;
}

@synthesize backgroundTopConstraint = _backgroundTopConstraint;
@synthesize delegate = _delegate;
@synthesize topToolbarSnapshotProvider = _topToolbarSnapshotProvider;
@synthesize bottomToolbarSnapshotProvider = _bottomToolbarSnapshotProvider;
@synthesize topMargin = _topMargin;

- (instancetype)initWithFrame:(CGRect)frame
                    topMargin:(CGFloat)topMargin
                        model:(TabModel*)model {
  self = [super initWithFrame:frame];
  if (self) {
    model_ = model;
    currentPoint_ = CGPointZero;
    _topMargin = topMargin;

    UIView* background = [[UIView alloc] initWithFrame:CGRectZero];
    [self addSubview:background];

    [background setTranslatesAutoresizingMaskIntoConstraints:NO];
    self.backgroundTopConstraint =
        [[background topAnchor] constraintEqualToAnchor:self.topAnchor
                                               constant:-StatusBarHeight()];
    [NSLayoutConstraint activateConstraints:@[
      [[background rightAnchor] constraintEqualToAnchor:self.rightAnchor],
      [[background leftAnchor] constraintEqualToAnchor:self.leftAnchor],
      self.backgroundTopConstraint,
      [[background bottomAnchor] constraintEqualToAnchor:self.bottomAnchor]
    ]];

    InstallBackgroundInView(background);

    _rightCard =
        [[SwipeView alloc] initWithFrame:CGRectZero topMargin:topMargin];
    _leftCard =
        [[SwipeView alloc] initWithFrame:CGRectZero topMargin:topMargin];
    [_rightCard setTranslatesAutoresizingMaskIntoConstraints:NO];
    [_leftCard setTranslatesAutoresizingMaskIntoConstraints:NO];
    [self addSubview:_rightCard];
    [self addSubview:_leftCard];
    AddSameConstraints(_rightCard, self);
    AddSameConstraints(_leftCard, self);
  }
  return self;
}

- (void)updateConstraints {
  [super updateConstraints];
  self.backgroundTopConstraint.constant = -StatusBarHeight();
}

- (CGRect)cardFrame {
  return self.bounds;
}

// Set up left and right card views depending on current tab and swipe
// direction.
- (void)updateViewsForDirection:(UISwipeGestureRecognizerDirection)direction {
  _direction = direction;
  CGRect cardFrame = [self cardFrame];
  NSUInteger currentIndex = [model_ indexOfTab:model_.currentTab];
  CGFloat offset = UseRTLLayout() ? -1 : 1;
  if (_direction == UISwipeGestureRecognizerDirectionRight) {
    [self setupCard:_rightCard withIndex:currentIndex];
    [_rightCard setFrame:cardFrame];
    [self setupCard:_leftCard withIndex:currentIndex - offset];
    cardFrame.origin.x -= cardFrame.size.width + kCardHorizontalSpacing;
    [_leftCard setFrame:cardFrame];
  } else {
    [self setupCard:_leftCard withIndex:currentIndex];
    [_leftCard setFrame:cardFrame];
    [self setupCard:_rightCard withIndex:currentIndex + offset];
    cardFrame.origin.x += cardFrame.size.width + kCardHorizontalSpacing;
    [_rightCard setFrame:cardFrame];
  }
}

- (UIImage*)smallGreyImage:(UIImage*)image {
  CGRect smallSize = CGRectMake(0, 0, image.size.width / kResizeFactor,
                                image.size.height / kResizeFactor);
  // Using CIFilter here on iOS 5+ might be faster, but it doesn't easily
  // allow for resizing.  At the max size, it's still too slow for side swipe.
  UIGraphicsBeginImageContextWithOptions(smallSize.size, YES, 0);
  [image drawInRect:smallSize blendMode:kCGBlendModeLuminosity alpha:1.0];
  UIImage* greyImage = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  return greyImage;
}

// Create card view based on TabModel index.
- (void)setupCard:(SwipeView*)card withIndex:(NSInteger)index {
  if (index < 0 || index >= (NSInteger)[model_ count]) {
    [card setHidden:YES];
    return;
  }
  [card setHidden:NO];

  Tab* tab = [model_ tabAtIndex:index];
  BOOL isNTP =
      tab.webState->GetLastCommittedURL().host_piece() == kChromeUINewTabHost;
  UIImage* topToolbarSnapshot = [self.topToolbarSnapshotProvider
      toolbarSideSwipeSnapshotForWebState:tab.webState];
  [card setTopToolbarImage:topToolbarSnapshot isNewTabPage:isNTP];
  UIImage* bottomToolbarSnapshot = [self.bottomToolbarSnapshotProvider
      toolbarSideSwipeSnapshotForWebState:tab.webState];
  [card setBottomToolbarImage:bottomToolbarSnapshot];

  // Converting snapshotted images to grey takes too much time for single core
  // devices.  Instead, show the colored image for single core devices and the
  // grey image for multi core devices.
  dispatch_queue_t priorityQueue =
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0ul);
  SnapshotTabHelper::FromWebState(tab.webState)
      ->RetrieveColorSnapshot(^(UIImage* image) {
        if (PagePlaceholderTabHelper::FromWebState(tab.webState)
                ->will_add_placeholder_for_next_navigation() &&
            !ios::device_util::IsSingleCoreDevice()) {
          [card setImage:SnapshotTabHelper::GetDefaultSnapshotImage()];
          dispatch_async(priorityQueue, ^{
            UIImage* greyImage = [self smallGreyImage:image];
            dispatch_async(dispatch_get_main_queue(), ^{
              [card setImage:greyImage];
            });
          });
        } else {
          [card setImage:image];
        }
      });
}

// Place cards around |currentPoint_.x|, and lean towards each card near the
// X edges of |bounds|.  Shrink cards as they are dragged towards the middle of
// the |bounds|, and edge cards only drag |kEdgeCardDragPercentage| of |bounds|.
- (void)updateCardPositions {
  CGRect bounds = [self cardFrame];
  [_rightCard setFrame:bounds];
  [_leftCard setFrame:bounds];

  CGFloat width = CGRectGetWidth([self cardFrame]);
  CGPoint center = CGPointMake(bounds.origin.x + bounds.size.width / 2,
                               bounds.origin.y + bounds.size.height / 2);
  if ([self isEdgeSwipe]) {
    // If an edge card, don't allow the card to be dragged across the screen.
    // Instead, drag across |kEdgeCardDragPercentage| of the screen.
    center.x = currentPoint_.x - width / 2 -
               (currentPoint_.x - width) / width *
                   (width * (1 - kEdgeCardDragPercentage));
    [_leftCard setCenter:center];
    center.x = currentPoint_.x / width * (width * kEdgeCardDragPercentage) +
               bounds.size.width / 2;
    [_rightCard setCenter:center];
  } else {
    // Place cards around the finger as it is dragged across the screen.
    // Place the finger between the cards in the middle of the screen, on the
    // right card border when on the left side of the screen, and on the left
    // card border when on the right side of the screen.
    CGFloat rightXBuffer = kCardHorizontalSpacing * currentPoint_.x / width;
    CGFloat leftXBuffer = kCardHorizontalSpacing - rightXBuffer;

    center.x = currentPoint_.x - leftXBuffer - width / 2;
    [_leftCard setCenter:center];

    center.x = currentPoint_.x + rightXBuffer + width / 2;
    [_rightCard setCenter:center];
  }
}

// Update layout with new touch event.
- (void)handleHorizontalPan:(SideSwipeGestureRecognizer*)gesture {
  currentPoint_ = [gesture locationInView:self];
  currentPoint_.x -= gesture.swipeOffset;

  // Since it's difficult to touch the very edge of the screen (touches tend to
  // sit near x ~ 4), push the touch towards the edge.
  CGFloat width = CGRectGetWidth([self cardFrame]);
  CGFloat half = floor(width / 2);
  CGFloat padding = floor(std::abs(currentPoint_.x - half) / half);

  // Push towards the edges.
  if (currentPoint_.x > half)
    currentPoint_.x += padding;
  else
    currentPoint_.x -= padding;

  // But don't go past the edges.
  if (currentPoint_.x < 0)
    currentPoint_.x = 0;
  else if (currentPoint_.x > width)
    currentPoint_.x = width;

  [self updateCardPositions];

  if (gesture.state == UIGestureRecognizerStateEnded ||
      gesture.state == UIGestureRecognizerStateCancelled ||
      gesture.state == UIGestureRecognizerStateFailed) {
    [self finishPan];
  }
}

- (BOOL)isEdgeSwipe {
  NSUInteger currentIndex = [model_ indexOfTab:model_.currentTab];
  return (IsSwipingBack(_direction) && currentIndex == 0) ||
         (IsSwipingForward(_direction) && currentIndex == [model_ count] - 1);
}

// Update the current tab and animate the proper card view if the
// |currentPoint_| is past the center of |bounds|.
- (void)finishPan {
  NSUInteger currentIndex = [model_ indexOfTab:model_.currentTab];
  // Something happened and now currentTab is gone.  End card side swipe and let
  // BVC show no tabs UI.
  if (currentIndex == NSNotFound)
    return [_delegate sideSwipeViewDismissAnimationDidEnd:self];

  CGRect finalSize = [self cardFrame];
  CGFloat width = CGRectGetWidth([self cardFrame]);
  CGRect leftFrame, rightFrame;
  SwipeView* dominantCard;
  Tab* destinationTab = model_.currentTab;
  CGFloat offset = UseRTLLayout() ? -1 : 1;
  if (_direction == UISwipeGestureRecognizerDirectionRight) {
    // If swipe is right and |currentPoint_.x| is over the first 1/3, move left.
    if (currentPoint_.x > width / 3.0 && ![self isEdgeSwipe]) {
      destinationTab = [model_ tabAtIndex:currentIndex - offset];
      dominantCard = _leftCard;
      rightFrame = leftFrame = finalSize;
      rightFrame.origin.x += rightFrame.size.width + kCardHorizontalSpacing;
      base::RecordAction(UserMetricsAction("MobileStackSwipeCompleted"));
    } else {
      dominantCard = _rightCard;
      leftFrame = rightFrame = finalSize;
      leftFrame.origin.x -= rightFrame.size.width + kCardHorizontalSpacing;
      base::RecordAction(UserMetricsAction("MobileStackSwipeCancelled"));
    }
  } else {
    // If swipe is left and |currentPoint_.x| is over the first 1/3, move right.
    if (currentPoint_.x < (width / 3.0) * 2.0 && ![self isEdgeSwipe]) {
      destinationTab = [model_ tabAtIndex:currentIndex + offset];
      dominantCard = _rightCard;
      leftFrame = rightFrame = finalSize;
      leftFrame.origin.x -= rightFrame.size.width + kCardHorizontalSpacing;
      base::RecordAction(UserMetricsAction("MobileStackSwipeCompleted"));
    } else {
      dominantCard = _leftCard;
      rightFrame = leftFrame = finalSize;
      rightFrame.origin.x += rightFrame.size.width + kCardHorizontalSpacing;
      base::RecordAction(UserMetricsAction("MobileStackSwipeCancelled"));
    }
  }

  if (destinationTab != model_.currentTab) {
    // The old tab is now hidden. The new tab will be inserted once the
    // animation is complete.
    model_.currentTab.webState->WasHidden();
  }

  // Make sure the dominant card animates on top.
  [dominantCard.superview bringSubviewToFront:dominantCard];

  [UIView animateWithDuration:kAnimationDuration
      animations:^{
        [_leftCard setTransform:CGAffineTransformIdentity];
        [_rightCard setTransform:CGAffineTransformIdentity];
        [_leftCard setFrame:leftFrame];
        [_rightCard setFrame:rightFrame];
      }
      completion:^(BOOL finished) {
        // Changing the model even when the tab is the same at the end of the
        // animation allows the UI to recover.
        [model_ setCurrentTab:destinationTab];
        [_leftCard setImage:nil];
        [_rightCard setImage:nil];
        [_leftCard setTopToolbarImage:nil isNewTabPage:NO];
        [_rightCard setTopToolbarImage:nil isNewTabPage:NO];
        [_leftCard setBottomToolbarImage:nil];
        [_rightCard setBottomToolbarImage:nil];
        [_delegate sideSwipeViewDismissAnimationDidEnd:self];
      }];
}

@end

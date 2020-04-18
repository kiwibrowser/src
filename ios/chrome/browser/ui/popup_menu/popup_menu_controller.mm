// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/popup_menu/popup_menu_controller.h"

#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#import "ios/chrome/browser/ui/animation_util.h"
#import "ios/chrome/browser/ui/popup_menu/popup_menu_view.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/common/material_timing.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using ios::material::TimingFunction;

namespace {
// Inset for the shadows of the contained views.
static const CGFloat kPopupMenuVerticalInset = 11.0;
// Duration for the Popup Menu Fade In animation
static const CGFloat kFadeInAnimationDuration = 0.2;
// Value to pass in for backgroundButtonTag to not set a tag value for
// |backgroundButton_|.
static const NSInteger kBackgroundButtonNoTag = -1;
// Default width of the popup container.
static const CGFloat kPopupContainerWidth = 236.0;
// Default height of the popup container.
static const CGFloat kPopupContainerHeight = 280.0;
static const CGFloat kPopoverScaleFactor = 0.03;

static void SetAnchorPoint(CGPoint anchorPoint, UIView* view) {
  CGPoint oldOrigin = view.frame.origin;
  view.layer.anchorPoint = anchorPoint;
  CGPoint newOrigin = view.frame.origin;

  CGPoint transition;
  transition.x = newOrigin.x - oldOrigin.x;
  transition.y = newOrigin.y - oldOrigin.y;

  view.center =
      CGPointMake(view.center.x - transition.x, view.center.y - transition.y);
}

static CGPoint AnimateInIntermediaryPoint(CGPoint source, CGPoint destination) {
  CGPoint midPoint = CGPointZero;
  midPoint.x = destination.x;
  midPoint.y = source.y - 0.8 * fabs(destination.y - source.y);
  return midPoint;
}

}  // anonymous namespace

@interface PopupMenuController ()<PopupMenuViewDelegate> {
  CGPoint sourceAnimationPoint_;
}
@end

@implementation PopupMenuController

@synthesize containerView = containerView_;
@synthesize backgroundButton = backgroundButton_;
@synthesize popupContainer = popupContainer_;
@synthesize delegate = delegate_;
@synthesize dispatcher = dispatcher_;

- (id)initWithParentView:(UIView*)parent {
  return [self initWithParentView:parent
           backgroundButtonParent:nil
            backgroundButtonColor:nil
            backgroundButtonAlpha:1.0
              backgroundButtonTag:kBackgroundButtonNoTag
         backgroundButtonSelector:nil];
}

- (id)initWithParentView:(UIView*)parent
      backgroundButtonParent:(UIView*)backgroundButtonParent
       backgroundButtonColor:(UIColor*)backgroundButtonColor
       backgroundButtonAlpha:(CGFloat)backgroundButtonAlpha
         backgroundButtonTag:(NSInteger)backgroundButtonTag
    backgroundButtonSelector:(SEL)backgroundButtonSelector {
  DCHECK(parent);
  self = [super init];
  if (self) {
    popupContainer_ = [[PopupMenuView alloc]
        initWithFrame:CGRectMake(0, 0, kPopupContainerWidth,
                                 kPopupContainerHeight)];

    containerView_ = [[UIView alloc] initWithFrame:[parent bounds]];
    containerView_.backgroundColor = [UIColor clearColor];
    [containerView_ setAccessibilityViewIsModal:YES];
    [popupContainer_ setDelegate:self];
    // All views are added to the |containerView_| that in turn is added to the
    // parent view. The Container View is needed to have a simple alpha
    // transition when the menu is displayed or hidden.
    [containerView_ addSubview:popupContainer_];
    [parent addSubview:containerView_];

    // Initialize backgroundButton_.
    UIView* buttonParent =
        backgroundButtonParent == nil ? containerView_ : backgroundButtonParent;
    backgroundButton_ = [[UIButton alloc] initWithFrame:[buttonParent bounds]];
    [buttonParent addSubview:backgroundButton_];
    if (buttonParent == containerView_)
      [buttonParent sendSubviewToBack:backgroundButton_];

    backgroundButton_.autoresizingMask =
        UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth |
        UIViewAutoresizingFlexibleBottomMargin |
        UIViewAutoresizingFlexibleLeadingMargin();
    backgroundButton_.alpha = backgroundButtonAlpha;
    if (backgroundButtonTag != kBackgroundButtonNoTag)
      backgroundButton_.tag = backgroundButtonTag;
    if (backgroundButtonColor != nil) {
      backgroundButton_.backgroundColor = backgroundButtonColor;
    }
    if (backgroundButtonSelector != nil) {
      [backgroundButton_ addTarget:nil
                            action:backgroundButtonSelector
                  forControlEvents:UIControlEventTouchDown];
    } else {
      [backgroundButton_ addTarget:self
                            action:@selector(tappedBehindPopup:)
                  forControlEvents:UIControlEventTouchDown];
    }
    [backgroundButton_ setAccessibilityLabel:l10n_util::GetNSString(
                                                 IDS_IOS_TOOLBAR_CLOSE_MENU)];
  }
  return self;
}

- (void)setOptimalSize:(CGSize)optimalSize atOrigin:(CGPoint)origin {
  CGRect popupFrame = [popupContainer_ bounds];
  popupFrame.size.width = optimalSize.width;
  popupFrame.size.height = 2 * kPopupMenuVerticalInset + optimalSize.height;

  // If the origin is on the right half of the screen, treat origin as the top-
  // right coordinate instead of top-left.
  CGFloat xOffset = origin.x > [containerView_ bounds].size.width / 2
                        ? origin.x - popupFrame.size.width
                        : origin.x;
  [popupContainer_ setFrame:CGRectOffset(popupFrame, xOffset, origin.y)];
}

- (void)fadeInPopupFromSource:(CGPoint)source
                toDestination:(CGPoint)destination
                   completion:(ProceduralBlock)completion {
  [self animateInFromPoint:source toPoint:destination completion:completion];
  UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification,
                                  containerView_);
}

- (void)dismissAnimatedWithCompletion:(void (^)(void))completion {
  [self animateOutToPoint:sourceAnimationPoint_ completion:completion];
  UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification,
                                  nil);
}

// Animate the view on screen (Fade in).
- (void)fadeInPopup:(void (^)(BOOL finished))completionBlock {
  [UIView animateWithDuration:kFadeInAnimationDuration
                        delay:0.0
                      options:UIViewAnimationOptionAllowUserInteraction
                   animations:^{
                     [containerView_ setAlpha:1.0];
                   }
                   completion:completionBlock];
}

- (void)dealloc {
  [popupContainer_ removeFromSuperview];
  [backgroundButton_ removeFromSuperview];
  [containerView_ removeFromSuperview];
}

- (void)tappedBehindPopup:(id)sender {
  [self dismissPopupMenu];
}

- (void)animateInFromPoint:(CGPoint)source
                   toPoint:(CGPoint)destination
                completion:(ProceduralBlock)completion {
  sourceAnimationPoint_ = source;

  // Set anchor to top right for top right destinations.
  NSUInteger anchorX =
      destination.x > [containerView_ bounds].size.width / 2 ? 1 : 0;
  SetAnchorPoint(CGPointMake(anchorX, 0), popupContainer_);

  CGPoint midPoint = AnimateInIntermediaryPoint(source, destination);

  CATransform3D scaleTransform =
      CATransform3DMakeScale(kPopoverScaleFactor, kPopoverScaleFactor, 1);

  NSValue* destinationScaleValue =
      [NSValue valueWithCATransform3D:CATransform3DIdentity];

  [CATransaction begin];
  [CATransaction setCompletionBlock:^{
    if (completion)
      completion();
  }];
  CABasicAnimation* scaleAnimation =
      [CABasicAnimation animationWithKeyPath:@"transform"];
  CAMediaTimingFunction* easeOut = TimingFunction(ios::material::CurveEaseOut);
  [scaleAnimation setFromValue:[NSValue valueWithCATransform3D:scaleTransform]];
  [scaleAnimation setToValue:destinationScaleValue];
  [scaleAnimation setTimingFunction:easeOut];
  [scaleAnimation setDuration:ios::material::kDuration1];

  CAKeyframeAnimation* positionAnimation =
      [CAKeyframeAnimation animationWithKeyPath:@"position"];
  [positionAnimation setValues:@[
    [NSValue valueWithCGPoint:source], [NSValue valueWithCGPoint:source],
    [NSValue valueWithCGPoint:midPoint], [NSValue valueWithCGPoint:destination]
  ]];
  [positionAnimation setTimingFunction:easeOut];
  [positionAnimation setDuration:ios::material::kDuration1];
  [positionAnimation setKeyTimes:@[ @0, @0.2, @0.5, @1 ]];

  CAMediaTimingFunction* linear = TimingFunction(ios::material::CurveLinear);
  CAAnimation* fadeAnimation = OpacityAnimationMake(0, 1);
  [fadeAnimation setDuration:ios::material::kDuration2];
  [fadeAnimation setTimingFunction:linear];
  [fadeAnimation setBeginTime:ios::material::kDuration2];

  CALayer* layer = [popupContainer_ layer];
  [layer addAnimation:AnimationGroupMake(
                          @[ scaleAnimation, positionAnimation, fadeAnimation ])
               forKey:@"popup-in"];
  [CATransaction commit];
}

- (void)animateOutToPoint:(CGPoint)destination
               completion:(void (^)(void))completion {
  CGPoint source = [[popupContainer_ layer] position];

  CGPoint midPoint = CGPointZero;
  midPoint.x = destination.x;
  midPoint.y = source.y - 0.8 * fabs(destination.x - source.x);

  CATransform3D scaleTransform =
      CATransform3DMakeScale(kPopoverScaleFactor, kPopoverScaleFactor, 1);

  CAMediaTimingFunction* easeIn = TimingFunction(ios::material::CurveEaseIn);
  [CATransaction begin];
  [CATransaction setAnimationTimingFunction:easeIn];
  [CATransaction setAnimationDuration:ios::material::kDuration2];
  [CATransaction setCompletionBlock:^{
    if (completion)
      completion();
  }];

  NSValue* sourceScaleValue =
      [NSValue valueWithCATransform3D:CATransform3DIdentity];

  CABasicAnimation* scaleAnimation =
      [CABasicAnimation animationWithKeyPath:@"transform"];
  [scaleAnimation setFromValue:sourceScaleValue];
  [scaleAnimation setToValue:[NSValue valueWithCATransform3D:scaleTransform]];

  CABasicAnimation* positionAnimation =
      [CABasicAnimation animationWithKeyPath:@"position"];
  [positionAnimation setFromValue:[NSValue valueWithCGPoint:source]];
  [positionAnimation setToValue:[NSValue valueWithCGPoint:destination]];

  CABasicAnimation* fadeAnimation =
      [CABasicAnimation animationWithKeyPath:@"opacity"];
  [fadeAnimation setFromValue:@1];
  [fadeAnimation setToValue:@0];

  CALayer* layer = [popupContainer_ layer];
  [layer addAnimation:AnimationGroupMake(
                          @[ scaleAnimation, positionAnimation, fadeAnimation ])
               forKey:@"out"];

  [CATransaction commit];
}

#pragma mark -
#pragma mark PopupMenuViewDelegate

- (void)dismissPopupMenu {
  [delegate_ dismissPopupMenu:self];
}

@end

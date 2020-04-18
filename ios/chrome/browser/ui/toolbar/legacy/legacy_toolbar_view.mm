// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/legacy/legacy_toolbar_view.h"

#include "ios/chrome/browser/ui/ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation LegacyToolbarView

@synthesize animatingTransition = animatingTransition_;
@synthesize hitTestBoundsContraintRelaxed = hitTestBoundsContraintRelaxed_;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    // The minimal width of the toolbar.
    // Must be less than or equal to the smallest UIWindow size Chrome can
    // be in, which as of M64 is 320.
    // Must be greater or equal than the sum of the minimal width (and
    // padding) of all of its subviews.
    const CGFloat kMinToolbarWidth = 200;

    // When the toolbar is not in the view hierarchy, UIKit sets the bounds to
    // CGRectZero. This results in the subviews of the toolbars which are
    // positioned with autoresizing masks to be moved.
    // This constraint enforces a minimum width.
    [self.widthAnchor constraintGreaterThanOrEqualToConstant:kMinToolbarWidth]
        .active = YES;
  }
  return self;
}

// Some views added to the toolbar have bounds larger than the toolbar bounds
// and still needs to receive touches. The overscroll actions view is one of
// those. That method is overridden in order to still perform hit testing on
// subviews that resides outside the toolbar bounds.
- (UIView*)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
  UIView* hitView = [super hitTest:point withEvent:event];
  if (hitView || !self.hitTestBoundsContraintRelaxed)
    return hitView;

  for (UIView* view in [[self subviews] reverseObjectEnumerator]) {
    if (!view.userInteractionEnabled || [view isHidden] || [view alpha] < 0.01)
      continue;
    const CGPoint convertedPoint = [view convertPoint:point fromView:self];
    if ([view pointInside:convertedPoint withEvent:event]) {
      hitView = [view hitTest:convertedPoint withEvent:event];
      if (hitView)
        break;
    }
  }
  return hitView;
}

- (id<CAAction>)actionForLayer:(CALayer*)layer forKey:(NSString*)event {
  // Don't allow UIView block-based animations if we're already performing
  // explicit transition animations.
  if (self.animatingTransition)
    return (id<CAAction>)[NSNull null];
  return [super actionForLayer:layer forKey:event];
}

@end

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/ntp/recent_tabs/views/disclosure_view.h"

#include "base/numerics/math_constants.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Animation duration for rotating the disclosure icon.
const NSTimeInterval kDisclosureIconRotateDuration = 0.25;

// Angles of the closure icon.
// The rotation animation privileges rotating using the smallest angle. Setting
// |kCollapsedIconAngle| to a value slightly less then 0 forces the animation to
// always happen in the same half-plane.
const CGFloat kCollapsedIconAngle = -0.00001;
const CGFloat kExpandedIconAngle = base::kPiFloat;

}  // anonymous namespace

@implementation DisclosureView

- (instancetype)init {
  UIImage* arrowImage = [[UIImage imageNamed:@"ntp_opentabs_recent_arrow"]
      imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  self = [super initWithImage:arrowImage];
  return self;
}

- (void)setTransformWhenCollapsed:(BOOL)collapsed animated:(BOOL)animated {
  CGFloat angle = collapsed ? kCollapsedIconAngle : kExpandedIconAngle;
  if (animated) {
    [UIView animateWithDuration:kDisclosureIconRotateDuration
                     animations:^{
                       self.transform = CGAffineTransformRotate(
                           CGAffineTransformIdentity, angle);
                     }];
  } else {
    self.transform = CGAffineTransformRotate(CGAffineTransformIdentity, angle);
  }
}

@end

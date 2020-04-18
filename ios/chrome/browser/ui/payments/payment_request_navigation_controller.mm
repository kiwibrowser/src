// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/payment_request_navigation_controller.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface PaymentRequestNavigationController ()<UIGestureRecognizerDelegate>

@end

@implementation PaymentRequestNavigationController

- (instancetype)initWithRootViewController:
    (UIViewController*)rootViewController {
  self = [super initWithRootViewController:rootViewController];
  return self;
}

- (void)viewDidLoad {
  // Since the navigation bar is hidden, the gesture to swipe to go back can
  // become inactive. Setting the delegate to self is an MDC workaround to have
  // it consistently work with AppBar.
  // https://github.com/material-components/material-components-ios/issues/720
  [self.interactivePopGestureRecognizer setDelegate:self];
}

#pragma mark - UIGestureRecognizerDelegate

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer*)gestureRecognizer {
  DCHECK_EQ(gestureRecognizer, self.interactivePopGestureRecognizer);
  return self.viewControllers.count > 1;
}

@end

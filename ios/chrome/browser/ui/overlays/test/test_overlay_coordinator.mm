// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/test/test_overlay_coordinator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TestOverlayCoordinator ()

// Dummy UIViewController that is presented when the coordinator is started.
@property(nonatomic, readonly) UIViewController* viewController;

@end

@implementation TestOverlayCoordinator
@synthesize cancelled = _cancelled;
@synthesize viewController = _viewController;

- (UIViewController*)viewController {
  if (!_viewController)
    _viewController = [[UIViewController alloc] init];
  return _viewController;
}

#pragma mark - OverlayCoordinator

- (void)cancelOverlay {
  _cancelled = YES;
}

@end

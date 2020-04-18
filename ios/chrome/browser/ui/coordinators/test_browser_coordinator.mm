// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/coordinators/test_browser_coordinator.h"

#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TestBrowserCoordinator () {
  // The test parent view controller.
  UIViewController* _viewController;
}
@end

@implementation TestBrowserCoordinator

- (instancetype)init {
  if (self = [super init])
    _viewController = [[UIViewController alloc] init];
  return self;
}

@end

@implementation TestBrowserCoordinator (Internal)

- (UIViewController*)viewController {
  return _viewController;
}

@end

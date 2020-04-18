// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/browser_container/browser_container_coordinator.h"

#import "base/logging.h"
#import "ios/chrome/browser/ui/browser_container/browser_container_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BrowserContainerCoordinator
@synthesize viewController = _viewController;

#pragma mark - ChromeCoordinator

- (void)start {
  DCHECK(self.browserState);
  DCHECK(!_viewController);
  _viewController = [[BrowserContainerViewController alloc] init];
  [super start];
}

- (void)stop {
  _viewController = nil;
  [super stop];
}

@end

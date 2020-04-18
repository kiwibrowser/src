// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main_content/main_content_coordinator.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/chrome/browser/ui/main_content/main_content_ui_broadcasting_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface MainContentCoordinator ()
// Starts or stops broadcasting UI state from |viewController|.
- (void)startBroadcastingUI;
- (void)stopBroadcastingUI;
@end

@implementation MainContentCoordinator

#pragma mark - Accessors

- (UIViewController<MainContentUI>*)viewController {
  // Subclasses implement.
  NOTREACHED();
  return nil;
}

#pragma mark - BrowserCoordinator

- (void)start {
  [super start];
  [self startBroadcastingUI];
}

- (void)stop {
  [self stopBroadcastingUI];
  [super stop];
}

- (void)childCoordinatorDidStart:(BrowserCoordinator*)childCoordinator {
  if ([childCoordinator isKindOfClass:[self class]])
    [self stopBroadcastingUI];
}

- (void)childCoordinatorWillStop:(BrowserCoordinator*)childCoordinator {
  if ([childCoordinator isKindOfClass:[self class]])
    [self startBroadcastingUI];
}

#pragma mark - Private

- (void)startBroadcastingUI {
  StartBroadcastingMainContentUI(self.viewController,
                                 self.browser->broadcaster());
}

- (void)stopBroadcastingUI {
  StopBroadcastingMainContentUI(self.browser->broadcaster());
}

@end

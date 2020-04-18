// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main/main_coordinator.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/main/main_containing_view_controller.h"
#include "ios/chrome/browser/ui/main/main_feature_flags.h"
#import "ios/chrome/browser/ui/main/main_presenting_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface MainCoordinator () {
  // Instance variables backing properties of the same name.
  // |_mainViewController| will be owned by |self.window|.
  __weak UIViewController<ViewControllerSwapping>* _mainViewController;
}

@end

@implementation MainCoordinator

#pragma mark - property implementation.

- (UIViewController*)mainViewController {
  return _mainViewController;
}

- (id<ViewControllerSwapping>)viewControllerSwapper {
  return _mainViewController;
}

#pragma mark - ChromeCoordinator implementation.

- (void)start {
  UIViewController<ViewControllerSwapping>* mainViewController = nil;
  if (TabSwitcherPresentsBVCEnabled()) {
    mainViewController = [[MainPresentingViewController alloc] init];
  } else {
    mainViewController = [[MainContainingViewController alloc] init];
  }
  CHECK(mainViewController);
  _mainViewController = mainViewController;
  self.window.rootViewController = self.mainViewController;
}

@end

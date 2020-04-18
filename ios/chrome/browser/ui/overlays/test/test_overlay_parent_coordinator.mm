// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/test/test_overlay_parent_coordinator.h"

#import <UIKit/UIKit.h>

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/chrome/browser/ui/overlays/overlay_coordinator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TestOverlayParentCoordinator ()
// The parent view controller's window.  Necessary for UIViewControllers to
// present properly.
@property(nonatomic, readonly, strong) UIWindow* window;
// The parent UIViewController.
@property(nonatomic, readonly, strong) UIViewController* viewController;
@end

@implementation TestOverlayParentCoordinator
@synthesize window = _window;
@synthesize viewController = _viewController;

- (instancetype)init {
  if ((self = [super init])) {
    _window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    _viewController = [[UIViewController alloc] init];
    _window.rootViewController = _viewController;
    [_window makeKeyAndVisible];
  }
  return self;
}

- (void)dealloc {
  [_window removeFromSuperview];
  [_window resignKeyWindow];
}

#pragma mark - Public

- (OverlayCoordinator*)presentedOverlay {
  NSUInteger childCount = self.children.count;
  DCHECK_LE(childCount, 1U);
  return childCount == 1U ? base::mac::ObjCCastStrict<OverlayCoordinator>(
                                [self.children anyObject])
                          : nil;
}

@end

@implementation TestOverlayParentCoordinator (Internal)

- (void)childCoordinatorDidStart:(BrowserCoordinator*)childCoordinator {
  [self.viewController presentViewController:childCoordinator.viewController
                                    animated:NO
                                  completion:nil];
}

- (void)childCoordinatorWillStop:(BrowserCoordinator*)childCoordinator {
  [self.viewController dismissViewControllerAnimated:NO completion:nil];
}

@end

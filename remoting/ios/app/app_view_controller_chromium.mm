// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/app/app_view_controller.h"

#include "base/logging.h"

#import "remoting/ios/app/remoting_menu_view_controller.h"

// The Chromium implementation for the AppViewController. It simply shows the
// menu modally.
@interface AppViewController () {
  UIViewController* _mainViewController;

  RemotingMenuViewController* _menuController;  // nullable
}
@end

@implementation AppViewController

- (instancetype)initWithMainViewController:
    (UIViewController*)mainViewController {
  if (self = [super init]) {
    _mainViewController = mainViewController;
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];

  [self addChildViewController:_mainViewController];
  [self.view addSubview:_mainViewController.view];
  [_mainViewController didMoveToParentViewController:self];
}

#pragma mark - AppController
- (void)showMenuAnimated:(BOOL)animated {
  if (_menuController != nil && [_menuController isBeingPresented]) {
    return;
  }
  _menuController = [[RemotingMenuViewController alloc] init];
  [self presentViewController:_menuController animated:animated completion:nil];
}

- (void)hideMenuAnimated:(BOOL)animated {
  if (_menuController == nil || ![_menuController isBeingPresented]) {
    return;
  }
  [_menuController dismissViewControllerAnimated:animated completion:nil];
  _menuController = nil;
}

- (void)presentSignInFlow {
  [self showMenuAnimated:YES];
}

- (UIViewController*)childViewControllerForStatusBarStyle {
  return _mainViewController;
}

- (UIViewController*)childViewControllerForStatusBarHidden {
  return _mainViewController;
}

@end

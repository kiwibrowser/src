// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main/main_view_controller_test.h"

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher.h"
#include "testing/gtest_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// TestTabSwitcherViewController is a UIViewController subclass that provides an
// empty implementation of the methods in the TabSwitcher protocol.  This is
// useful for unittests.
@interface TestTabSwitcherViewController : UIViewController<TabSwitcher>
@end

@implementation TestTabSwitcherViewController

@synthesize animationDelegate = _animationDelegate;
@synthesize delegate = _delegate;
@synthesize dispatcher = _dispatcher;

- (void)restoreInternalStateWithMainTabModel:(TabModel*)mainModel
                                 otrTabModel:(TabModel*)otrModel
                              activeTabModel:(TabModel*)activeModel {
}

- (UIViewController*)viewController {
  return self;
}

- (void)showWithSelectedTabAnimation {
}

- (Tab*)dismissWithNewTabAnimationToModel:(TabModel*)targetModel
                                  withURL:(const GURL&)url
                                  atIndex:(NSUInteger)position
                               transition:(ui::PageTransition)transition {
  return nil;
}

- (void)setOtrTabModel:(TabModel*)otrModel {
}

- (void)prepareForDisplayAtSize:(CGSize)size {
}

@end

id<TabSwitcher> MainViewControllerTest::CreateTestTabSwitcher() {
  return [[TestTabSwitcherViewController alloc] init];
}

// Sets the current key window's rootViewController and saves a pointer to
// the original VC to allow restoring it at the end of the test.
void MainViewControllerTest::SetRootViewController(
    UIViewController* new_root_view_controller) {
  original_root_view_controller_ =
      [[[UIApplication sharedApplication] keyWindow] rootViewController];
  [[UIApplication sharedApplication] keyWindow].rootViewController =
      new_root_view_controller;
}

void MainViewControllerTest::TearDown() {
  if (original_root_view_controller_) {
    [[UIApplication sharedApplication] keyWindow].rootViewController =
        original_root_view_controller_;
    original_root_view_controller_ = nil;
  }
}

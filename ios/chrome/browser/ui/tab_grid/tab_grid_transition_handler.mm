// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_grid/tab_grid_transition_handler.h"

#import "ios/chrome/browser/ui/tab_grid/transitions/grid_to_hidden_tab_animator.h"
#import "ios/chrome/browser/ui/tab_grid/transitions/grid_to_visible_tab_animator.h"
#import "ios/chrome/browser/ui/tab_grid/transitions/grid_transition_state_providing.h"
#import "ios/chrome/browser/ui/tab_grid/transitions/tab_to_grid_animator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation TabGridTransitionHandler

@synthesize provider = _provider;

#pragma mark - UIViewControllerTransitioningDelegate

- (id<UIViewControllerAnimatedTransitioning>)
animationControllerForPresentedController:(UIViewController*)presented
                     presentingController:(UIViewController*)presenting
                         sourceController:(UIViewController*)source {
  id<UIViewControllerAnimatedTransitioning> animator;
  if (self.provider.selectedCellVisible) {
    // This will be a GridToVisibleTabAnimator eventually.
    animator =
        [[GridToVisibleTabAnimator alloc] initWithStateProvider:self.provider];
  } else {
    animator = [[GridToHiddenTabAnimator alloc] init];
  }
  return animator;
}

- (id<UIViewControllerAnimatedTransitioning>)
animationControllerForDismissedController:(UIViewController*)dismissed {
  return [[TabToGridAnimator alloc] initWithStateProvider:self.provider];
}

@end

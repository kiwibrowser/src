// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_grid/transitions/grid_to_hidden_tab_animator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation GridToHiddenTabAnimator

- (NSTimeInterval)transitionDuration:
    (id<UIViewControllerContextTransitioning>)transitionContext {
  return 0.25;
}

- (void)animateTransition:
    (id<UIViewControllerContextTransitioning>)transitionContext {
  // Get views and view controllers for this transition.
  UIView* containerView = [transitionContext containerView];
  UIViewController* tabViewController = [transitionContext
      viewControllerForKey:UITransitionContextToViewControllerKey];
  // The grid view, already in the view hierarchy.
  UIView* gridView =
      [transitionContext viewForKey:UITransitionContextFromViewKey];
  // The tab view, not yet in the view hierarchy.
  UIView* tabView = [transitionContext viewForKey:UITransitionContextToViewKey];

  // Add the tab view to the container view with the correct size.
  tabView.frame =
      [transitionContext finalFrameForViewController:tabViewController];
  [containerView addSubview:tabView];

  // The animation here creates a simple quick zoom effect -- the tab view
  // fades in as it expands to full size. The zoom is not large (80% to 100%)
  // and is centered on the view's final center position, so it's not directly
  // connected to any tab grid positions.

  // Set up the tab view for animation by giving it a scale transform and making
  // its alpha zero.
  tabView.alpha = 0;
  CGAffineTransform baseTabTransform = tabView.transform;
  tabView.transform = CGAffineTransformScale(baseTabTransform, 0.8, 0.8);

  // Animate the tab to full visibility and scale, then
  // clean up by removing the grid view.
  [UIView animateWithDuration:[self transitionDuration:transitionContext]
      delay:0.0
      options:UIViewAnimationOptionCurveEaseOut
      animations:^{
        tabView.alpha = 1.0;
        tabView.transform = baseTabTransform;
      }
      completion:^(BOOL finished) {
        // If the transition was cancelled, remove the tab view.
        // If not, remove the grid view.
        if (transitionContext.transitionWasCancelled) {
          [tabView removeFromSuperview];
        } else {
          [gridView removeFromSuperview];
        }
        // Mark the transition as completed.
        [transitionContext completeTransition:YES];
      }];
}
@end

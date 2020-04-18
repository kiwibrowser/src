// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_grid/transitions/grid_to_visible_tab_animator.h"

#import "ios/chrome/browser/ui/tab_grid/transitions/grid_transition_animation.h"
#import "ios/chrome/browser/ui/tab_grid/transitions/grid_transition_state_providing.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface GridToVisibleTabAnimator ()<GridTransitionAnimationDelegate>
@property(nonatomic, weak) id<GridTransitionStateProviding> stateProvider;
// Animation object for this transition.
@property(nonatomic, strong) GridTransitionAnimation* animation;
// Transition context passed into this object when the animation is started.
@property(nonatomic, weak) id<UIViewControllerContextTransitioning>
    transitionContext;
@end

@implementation GridToVisibleTabAnimator
@synthesize stateProvider = _stateProvider;
@synthesize animation = _animation;
@synthesize transitionContext = _transitionContext;

- (instancetype)initWithStateProvider:
    (id<GridTransitionStateProviding>)stateProvider {
  if ((self = [super init])) {
    _stateProvider = stateProvider;
  }
  return self;
}

- (NSTimeInterval)transitionDuration:
    (id<UIViewControllerContextTransitioning>)transitionContext {
  return 0.4;
}

- (void)animateTransition:
    (id<UIViewControllerContextTransitioning>)transitionContext {
  // Keep a pointer to the transition context for use in animation delegate
  // callbacks.
  self.transitionContext = transitionContext;

  // Get views and view controllers for this transition.
  UIView* containerView = [transitionContext containerView];
  UIViewController* presentedViewController = [transitionContext
      viewControllerForKey:UITransitionContextToViewControllerKey];
  UIView* presentedView =
      [transitionContext viewForKey:UITransitionContextToViewKey];

  // Add the presented view to the container. This isn't just for the
  // transition; this is how the presented view controller's view is added to
  // the view hierarchy.
  [containerView addSubview:presentedView];
  presentedView.frame =
      [transitionContext finalFrameForViewController:presentedViewController];
  presentedView.alpha = 0.0;

  // Get the layout of the grid for the transition.
  GridTransitionLayout* layout =
      [self.stateProvider layoutForTransitionContext:transitionContext];

  // Create the animation view and insert it.
  self.animation = [[GridTransitionAnimation alloc]
      initWithLayout:layout
            delegate:self
           direction:GridAnimationDirectionExpanding];

  //   Ask the state provider for the views to use when inserting the animation.
  UIView* proxyContainer =
      [self.stateProvider proxyContainerForTransitionContext:transitionContext];
  UIView* viewBehindProxies =
      [self.stateProvider proxyPositionForTransitionContext:transitionContext];

  [proxyContainer insertSubview:self.animation aboveSubview:viewBehindProxies];

  NSTimeInterval duration = [self transitionDuration:transitionContext];

  // Fade in the presented view.
  [UIView animateWithDuration:duration * 0.3
                        delay:duration * 0.7
                      options:UIViewAnimationOptionCurveEaseOut
                   animations:^{
                     presentedView.alpha = 1.0;
                   }
                   completion:nil];

  // Run the main animation.
  [self.animation animateWithDuration:duration];
}

- (void)gridTransitionAnimationDidFinish:(BOOL)finished {
  // Clean up the animation
  [self.animation removeFromSuperview];
  // If the transition was cancelled, remove the presented view.
  // If not, remove the grid view.
  UIView* gridView =
      [self.transitionContext viewForKey:UITransitionContextFromViewKey];
  UIView* presentedView =
      [self.transitionContext viewForKey:UITransitionContextToViewKey];
  if (self.transitionContext.transitionWasCancelled) {
    [presentedView removeFromSuperview];
  } else {
    [gridView removeFromSuperview];
  }
  // Mark the transition as completed.
  [self.transitionContext completeTransition:YES];
}

@end

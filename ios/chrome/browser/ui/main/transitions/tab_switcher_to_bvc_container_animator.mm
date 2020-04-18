// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main/transitions/tab_switcher_to_bvc_container_animator.h"

#import "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TabSwitcherToBVCContainerAnimator ()<TabSwitcherAnimationDelegate>

@property(nonatomic, readwrite, weak) id<UIViewControllerContextTransitioning>
    transitionContext;

@end

@implementation TabSwitcherToBVCContainerAnimator

@synthesize tabSwitcher = _tabSwitcher;
@synthesize transitionContext = _transitionContext;

- (NSTimeInterval)transitionDuration:
    (id<UIViewControllerContextTransitioning>)transitionContext {
  // This value is arbitrary, chosen to roughly match the visual length of the
  // stack view animations.  The returned value does not appear to be used
  // anywhere.  The actual transition does not complete until
  // |tabSwitcherDismissalAnimationDidEnd:| is called, which happens as a
  // result of a CoreAnimation completion block.
  return 0.25;
}

// Tab switcher dismissal animations are currently driven by the tab switcher
// itself.  MainController and this animator object are notified of progress
// through delegate callbacks, but they do not directly participate in the
// animation.  As a result, this animator is largely along for the ride.  It
// does not initiate or control any animations, and its main responsibility is
// to call |completeTransition| when the tab switcher is finished animating.
- (void)animateTransition:
    (id<UIViewControllerContextTransitioning>)transitionContext {
  UIViewController* fromViewController = [transitionContext
      viewControllerForKey:UITransitionContextFromViewControllerKey];
  UIViewController* toViewController = [transitionContext
      viewControllerForKey:UITransitionContextToViewControllerKey];

  UIView* containerView = transitionContext.containerView;
  UIView* fromView =
      [transitionContext viewForKey:UITransitionContextFromViewKey];
  UIView* toView = [transitionContext viewForKey:UITransitionContextToViewKey];
  fromView.frame =
      [transitionContext initialFrameForViewController:fromViewController];
  toView.frame =
      [transitionContext finalFrameForViewController:toViewController];

  // This animator is responsible for adding the incoming view to the
  // containerView for the presentation/dismissal.  Since the animations run in
  // the tab switcher's view, order the subviews to ensure that the tab
  // switcher's view remains on top.
  [containerView addSubview:toView];
  [containerView sendSubviewToBack:toView];

  DCHECK_EQ(fromViewController,
            [self.tabSwitcher viewController].parentViewController);
  self.tabSwitcher.animationDelegate = self;
  self.transitionContext = transitionContext;
}

#pragma mark - TabSwitcherAnimationDelegate

- (void)tabSwitcherPresentationAnimationDidEnd:(id<TabSwitcher>)tabSwitcher {
  // This animator does not expect to participate in presentation animations, so
  // it is an error if this method ever gets called.
  NOTREACHED();
}

- (void)tabSwitcherDismissalAnimationDidEnd:(id<TabSwitcher>)tabSwitcher {
  // Calling |completeTransition:| seems to deallocate |self|, so make any
  // necessary changes to |self| here, and be sure not to access |self| after
  // the call to |completeTransition:|.
  id<UIViewControllerContextTransitioning> transitionContext =
      self.transitionContext;
  self.tabSwitcher = nil;
  self.transitionContext = nil;

  tabSwitcher.animationDelegate = nil;
  BOOL wasCancelled = [transitionContext transitionWasCancelled];
  [transitionContext completeTransition:!wasCancelled];
}

@end

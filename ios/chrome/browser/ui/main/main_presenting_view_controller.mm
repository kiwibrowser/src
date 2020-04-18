// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main/main_presenting_view_controller.h"

#import "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/main/bvc_container_view_controller.h"
#import "ios/chrome/browser/ui/main/transitions/bvc_container_to_tab_switcher_animator.h"
#import "ios/chrome/browser/ui/main/transitions/tab_switcher_to_bvc_container_animator.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface MainPresentingViewController ()<
    UIViewControllerTransitioningDelegate>

@property(nonatomic, strong) BVCContainerViewController* bvcContainer;

// A copy of the launch screen view used to mask flicker at startup.
@property(nonatomic, strong) UIView* launchScreen;

// Redeclared as readwrite.
@property(nonatomic, readwrite, weak) id<TabSwitcher> tabSwitcher;

@end

@implementation MainPresentingViewController
@synthesize animationsDisabledForTesting = _animationsDisabledForTesting;
@synthesize tabSwitcher = _tabSwitcher;
@synthesize bvcContainer = _bvcContainer;
@synthesize launchScreen = _launchScreen;

- (void)viewDidLoad {
  // Set a white background color to avoid flickers of black during startup.
  self.view.backgroundColor = [UIColor whiteColor];
  // In some circumstances (such as when uploading a crash report), there may
  // be no other view controller visible for a few seconds. To prevent a
  // white screen from appearing in that case, the startup view is added to
  // the view hierarchy until another view controller is added.
  NSBundle* mainBundle = base::mac::FrameworkBundle();
  NSArray* topObjects =
      [mainBundle loadNibNamed:@"LaunchScreen" owner:self options:nil];
  UIViewController* launchScreenController =
      base::mac::ObjCCastStrict<UIViewController>([topObjects lastObject]);
  self.launchScreen = launchScreenController.view;
  self.launchScreen.userInteractionEnabled = NO;
  self.launchScreen.frame = self.view.bounds;
  [self.view addSubview:self.launchScreen];
}

- (UIViewController*)activeViewController {
  if (self.bvcContainer) {
    DCHECK_EQ(self.bvcContainer, self.presentedViewController);
    DCHECK(self.bvcContainer.currentBVC);
    return self.bvcContainer.currentBVC;
  } else if (self.tabSwitcher) {
    DCHECK_EQ(self.tabSwitcher, [self.childViewControllers firstObject]);
    return [self.tabSwitcher viewController];
  }

  return nil;
}

- (UIViewController*)viewController {
  return self;
}

- (void)showTabSwitcher:(id<TabSwitcher>)tabSwitcher
             completion:(ProceduralBlock)completion {
  DCHECK(tabSwitcher);

  // Before any child view controller would be added, remove the launch screen.
  if (self.launchScreen) {
    [self.launchScreen removeFromSuperview];
    self.launchScreen = nil;
  }

  // Don't remove and re-add the tabSwitcher if it hasn't changed.
  if (self.tabSwitcher != tabSwitcher) {
    // Remove any existing tab switchers first.
    if (self.tabSwitcher) {
      UIViewController* tabSwitcherViewController =
          [tabSwitcher viewController];
      [tabSwitcherViewController willMoveToParentViewController:nil];
      [tabSwitcherViewController.view removeFromSuperview];
      [tabSwitcherViewController removeFromParentViewController];
    }

    // Reset the background color of the container view.  The tab switcher does
    // not draw anything below the status bar, so those pixels fall through to
    // display the container's background.
    self.view.backgroundColor = [UIColor clearColor];

    UIViewController* tabSwitcherViewController = [tabSwitcher viewController];
    // Add the new tab switcher as a child VC.
    [self addChildViewController:tabSwitcherViewController];
    tabSwitcherViewController.view.translatesAutoresizingMaskIntoConstraints =
        NO;
    [self.view addSubview:tabSwitcherViewController.view];

    [NSLayoutConstraint activateConstraints:@[
      [tabSwitcherViewController.view.topAnchor
          constraintEqualToAnchor:self.view.topAnchor],
      [tabSwitcherViewController.view.bottomAnchor
          constraintEqualToAnchor:self.view.bottomAnchor],
      [tabSwitcherViewController.view.leadingAnchor
          constraintEqualToAnchor:self.view.leadingAnchor],
      [tabSwitcherViewController.view.trailingAnchor
          constraintEqualToAnchor:self.view.trailingAnchor],
    ]];

    [tabSwitcherViewController didMoveToParentViewController:self];
    self.tabSwitcher = tabSwitcher;
  }

  // If a BVC is currently being presented, dismiss it.  This will trigger any
  // necessary animations.
  if (self.bvcContainer) {
    // Pre-size the tab switcher's view if necessary.
    [self.tabSwitcher
        prepareForDisplayAtSize:self.bvcContainer.view.bounds.size];
    self.bvcContainer.transitioningDelegate = self;
    self.bvcContainer = nil;
    BOOL animated = !self.animationsDisabledForTesting;
    [super dismissViewControllerAnimated:animated completion:completion];
  } else {
    if (completion) {
      completion();
    }
  }
}

- (void)showTabViewController:(UIViewController*)viewController
                   completion:(ProceduralBlock)completion {
  DCHECK(viewController);

  // If another BVC is already being presented, swap this one into the
  // container.
  if (self.bvcContainer) {
    self.bvcContainer.currentBVC = viewController;
    if (completion) {
      completion();
    }
    return;
  }

  self.bvcContainer = [[BVCContainerViewController alloc] init];
  self.bvcContainer.currentBVC = viewController;
  self.bvcContainer.transitioningDelegate = self;
  BOOL animated = !self.animationsDisabledForTesting && self.tabSwitcher != nil;
  [super presentViewController:self.bvcContainer
                      animated:animated
                    completion:completion];
}

#pragma mark - UIViewController methods

- (void)presentViewController:(UIViewController*)viewControllerToPresent
                     animated:(BOOL)flag
                   completion:(void (^)())completion {
  // If there is no activeViewController then this call will get inadvertently
  // dropped.
  DCHECK(self.activeViewController);
  [self.activeViewController presentViewController:viewControllerToPresent
                                          animated:flag
                                        completion:completion];
}

- (void)dismissViewControllerAnimated:(BOOL)flag
                           completion:(void (^)())completion {
  // If there is no activeViewController then this call will get inadvertently
  // dropped.
  DCHECK(self.activeViewController);
  [self.activeViewController dismissViewControllerAnimated:flag
                                                completion:completion];
}

- (UIViewController*)childViewControllerForStatusBarHidden {
  return self.activeViewController;
}

- (UIViewController*)childViewControllerForStatusBarStyle {
  return self.activeViewController;
}

- (BOOL)shouldAutorotate {
  return self.activeViewController
             ? [self.activeViewController shouldAutorotate]
             : [super shouldAutorotate];
}

#pragma mark - Transitioning Delegate

- (id<UIViewControllerAnimatedTransitioning>)
animationControllerForPresentedController:(UIViewController*)presented
                     presentingController:(UIViewController*)presenting
                         sourceController:(UIViewController*)source {
  TabSwitcherToBVCContainerAnimator* animator =
      [[TabSwitcherToBVCContainerAnimator alloc] init];
  animator.tabSwitcher = self.tabSwitcher;
  return animator;
}

- (id<UIViewControllerAnimatedTransitioning>)
animationControllerForDismissedController:(UIViewController*)dismissed {
  // Verify that the presenting and dismissed view controllers are of the
  // expected types.
  DCHECK([dismissed isKindOfClass:[BVCContainerViewController class]]);
  DCHECK([dismissed.presentingViewController
      isKindOfClass:[MainPresentingViewController class]]);

  BVCContainerToTabSwitcherAnimator* animator =
      [[BVCContainerToTabSwitcherAnimator alloc] init];
  animator.tabSwitcher = self.tabSwitcher;
  return animator;
}

@end

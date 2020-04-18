// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main/main_containing_view_controller.h"

#import "base/logging.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation MainContainingViewController

- (UIViewController*)activeViewController {
  return [self.childViewControllers firstObject];
}

- (UIViewController*)viewController {
  return self;
}

- (void)showTabSwitcher:(id<TabSwitcher>)tabSwitcher
             completion:(ProceduralBlock)completion {
  [self setActiveViewController:[tabSwitcher viewController]
                     completion:completion];
  [tabSwitcher showWithSelectedTabAnimation];
}

- (void)showTabViewController:(UIViewController*)viewController
                   completion:(ProceduralBlock)completion {
  [self setActiveViewController:viewController completion:completion];
}

// Swaps in and displays the given view controller, replacing any other
// view controllers that may currently be visible.  Runs
// the given |completion| block after the view controller is visible.
- (void)setActiveViewController:(UIViewController*)activeViewController
                     completion:(void (^)())completion {
  DCHECK(activeViewController);
  if (self.activeViewController == activeViewController) {
    if (completion) {
      completion();
    }
    return;
  }

  // TODO(crbug.com/546189): DCHECK here that there isn't a modal view
  // controller showing once the known violations of that are fixed.

  // Remove the current active view controller, if any.
  if (self.activeViewController) {
    [self.activeViewController willMoveToParentViewController:nil];
    [self.activeViewController.view removeFromSuperview];
    [self.activeViewController removeFromParentViewController];
  }

  DCHECK_EQ(nil, self.activeViewController);
  DCHECK_EQ(0U, self.view.subviews.count);

  // Add the new active view controller.
  [self addChildViewController:activeViewController];
  self.activeViewController.view.frame = self.view.bounds;
  [self.view addSubview:self.activeViewController.view];
  [activeViewController didMoveToParentViewController:self];

  // Let the system know that the child has changed so appearance updates can
  // be made.
  [self setNeedsStatusBarAppearanceUpdate];

  DCHECK(self.activeViewController == activeViewController);
  if (completion) {
    completion();
  }
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

@end

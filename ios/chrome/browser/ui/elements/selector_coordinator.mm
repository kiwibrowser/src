// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/elements/selector_coordinator.h"

#import "ios/chrome/browser/ui/elements/selector_picker_view_controller.h"
#import "ios/chrome/browser/ui/elements/selector_picker_presentation_controller.h"
#import "ios/chrome/browser/ui/elements/selector_view_controller_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface SelectorCoordinator ()<SelectorViewControllerDelegate,
                                  UIViewControllerTransitioningDelegate>

// Redeclaration of infoBarPickerController as readwrite.
@property(nonatomic, nullable, strong)
    SelectorPickerViewController* selectorPickerViewController;

@end

@implementation SelectorCoordinator

@synthesize options = _options;
@synthesize defaultOption = _defaultOption;
@synthesize delegate = _delegate;
@synthesize selectorPickerViewController = _selectorPickerViewController;

- (void)start {
  self.selectorPickerViewController =
      [[SelectorPickerViewController alloc] initWithOptions:self.options
                                                    default:self.defaultOption];
  self.selectorPickerViewController.delegate = self;

  self.selectorPickerViewController.modalTransitionStyle =
      UIModalTransitionStyleCoverVertical;
  self.selectorPickerViewController.modalPresentationStyle =
      UIModalPresentationCustom;
  self.selectorPickerViewController.transitioningDelegate = self;

  [self.baseViewController
      presentViewController:self.selectorPickerViewController
                   animated:YES
                 completion:nil];
}

- (void)stop {
  [self.selectorPickerViewController.presentingViewController
      dismissViewControllerAnimated:YES
                         completion:nil];
}

#pragma mark SelectorViewControllerDelegate

- (void)selectorViewController:(UIViewController*)viewController
               didSelectOption:(NSString*)option {
  [self.delegate selectorCoordinator:self didCompleteWithSelection:option];
  [self stop];
}

#pragma mark UIViewControllerTransitioningDelegate

- (UIPresentationController*)
presentationControllerForPresentedViewController:(UIViewController*)presented
                        presentingViewController:(UIViewController*)presenting
                            sourceViewController:(UIViewController*)source {
  return [[SelectorPickerPresentationController alloc]
      initWithPresentedViewController:self.selectorPickerViewController
             presentingViewController:self.baseViewController];
}

@end

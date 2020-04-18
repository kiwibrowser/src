// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_coordinator.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_mediator.h"
#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_view_controller.h"
#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_view_controller_presentation_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface IdentityChooserCoordinator ()<
    IdentityChooserViewControllerPresentationDelegate>

@property(nonatomic, strong) IdentityChooserMediator* identityChooserMediator;
@property(nonatomic, strong)
    IdentityChooserViewController* identityChooserViewController;

@end

@implementation IdentityChooserCoordinator

@synthesize delegate = _delegate;
@synthesize identityChooserMediator = _identityChooserMediator;
@synthesize identityChooserViewController = _identityChooserViewController;

- (void)start {
  [super start];
  // Creates the controller.
  self.identityChooserViewController =
      [[IdentityChooserViewController alloc] init];
  // Creates the mediator.
  self.identityChooserMediator = [[IdentityChooserMediator alloc] init];
  self.identityChooserMediator.identityChooserViewController =
      self.identityChooserViewController;
  // Setups.
  self.identityChooserViewController.presentationDelegate = self;
  self.identityChooserViewController.selectionDelegate =
      self.identityChooserMediator;
  // Starts.
  [self.identityChooserMediator start];
  [self.baseViewController
      presentViewController:self.identityChooserViewController
                   animated:YES
                 completion:nil];
}

#pragma mark - Setters/Getters

- (void)setSelectedIdentity:(ChromeIdentity*)selectedIdentity {
  self.identityChooserMediator.selectedIdentity = selectedIdentity;
}

- (ChromeIdentity*)selectedIdentity {
  return self.identityChooserMediator.selectedIdentity;
}

#pragma mark - IdentityChooserViewControllerPresentationDelegate

- (void)identityChooserViewControllerDidDisappear:
    (IdentityChooserViewController*)viewController {
  DCHECK_EQ(self.identityChooserViewController, viewController);
  [self.delegate identityChooserCoordinatorDidClose:self];
}

@end

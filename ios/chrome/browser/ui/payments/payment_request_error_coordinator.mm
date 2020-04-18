// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/payment_request_error_coordinator.h"

#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface PaymentRequestErrorCoordinator () {
  PaymentRequestErrorViewController* _viewController;
}

@end

@implementation PaymentRequestErrorCoordinator

@synthesize callback = _callback;
@synthesize delegate = _delegate;

- (void)start {
  _viewController = [[PaymentRequestErrorViewController alloc] init];
  [_viewController
      setErrorMessage:l10n_util::GetNSString(IDS_PAYMENTS_ERROR_MESSAGE)];
  [_viewController setDelegate:self];
  [_viewController loadModel];

  [[self baseViewController] presentViewController:_viewController
                                          animated:YES
                                        completion:nil];
}

- (void)stop {
  [[_viewController presentingViewController]
      dismissViewControllerAnimated:YES
                         completion:nil];
  _viewController = nil;
}

#pragma mark - PaymentRequestErrorViewControllerDelegate

- (void)paymentRequestErrorViewControllerDidDismiss:
    (PaymentRequestErrorViewController*)controller {
  [_delegate paymentRequestErrorCoordinatorDidDismiss:self];
}

@end

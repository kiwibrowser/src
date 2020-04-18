// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/shipping_option_selection_coordinator.h"

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "components/payments/core/payment_shipping_option.h"
#include "components/payments/core/web_payment_request.h"
#include "ios/chrome/browser/payments/payment_request.h"
#import "ios/chrome/browser/payments/payment_request_util.h"
#include "ios/chrome/browser/ui/payments/shipping_option_selection_mediator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
using ::payment_request_util::GetShippingOptionSelectorErrorMessage;

// The delay in nano seconds before notifying the delegate of the selection.
const int64_t kDelegateNotificationDelayInNanoSeconds = 0.2 * NSEC_PER_SEC;
}  // namespace

@interface ShippingOptionSelectionCoordinator ()

@property(nonatomic, strong)
    PaymentRequestSelectorViewController* viewController;

@property(nonatomic, strong) ShippingOptionSelectionMediator* mediator;

// Called when the user selects a shipping option. The cell is checked, the
// UI is locked so that the user can't interact with it, then the delegate is
// notified. The delay is here to let the user get a visual feedback of the
// selection before this view disappears.
- (void)delayedNotifyDelegateOfSelection:
    (payments::PaymentShippingOption*)shippingOption;

@end

@implementation ShippingOptionSelectionCoordinator

@synthesize paymentRequest = _paymentRequest;
@synthesize delegate = _delegate;
@synthesize viewController = _viewController;
@synthesize mediator = _mediator;

- (void)start {
  self.mediator = [[ShippingOptionSelectionMediator alloc]
      initWithPaymentRequest:self.paymentRequest];

  self.viewController = [[PaymentRequestSelectorViewController alloc] init];
  self.viewController.delegate = self;
  self.viewController.dataSource = self.mediator;
  [self.viewController loadModel];

  DCHECK(self.baseViewController.navigationController);
  [self.baseViewController.navigationController
      pushViewController:self.viewController
                animated:YES];
}

- (void)stop {
  [self.baseViewController.navigationController popViewControllerAnimated:YES];
  self.viewController = nil;
  self.mediator = nil;
}

- (void)stopSpinnerAndDisplayError {
  // Re-enable user interactions that were disabled earlier in
  // delayedNotifyDelegateOfSelection.
  self.viewController.view.userInteractionEnabled = YES;

  DCHECK(self.paymentRequest);
  self.mediator.headerText =
      GetShippingOptionSelectorErrorMessage(*self.paymentRequest);
  self.mediator.state = PaymentRequestSelectorStateError;
  [self.viewController loadModel];
  [self.viewController.collectionView reloadData];
}

#pragma mark - PaymentRequestSelectorViewControllerDelegate

- (BOOL)paymentRequestSelectorViewController:
            (PaymentRequestSelectorViewController*)controller
                        didSelectItemAtIndex:(NSUInteger)index {
  // Update the data source with the selection.
  self.mediator.selectedItemIndex = index;

  DCHECK(index < self.paymentRequest->shipping_options().size());
  [self delayedNotifyDelegateOfSelection:self.paymentRequest
                                             ->shipping_options()[index]];
  return YES;
}

- (void)paymentRequestSelectorViewControllerDidFinish:
    (PaymentRequestSelectorViewController*)controller {
  [self.delegate shippingOptionSelectionCoordinatorDidReturn:self];
}

#pragma mark - Helper methods

- (void)delayedNotifyDelegateOfSelection:
    (payments::PaymentShippingOption*)shippingOption {
  self.viewController.view.userInteractionEnabled = NO;
  __weak ShippingOptionSelectionCoordinator* weakSelf = self;
  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW, kDelegateNotificationDelayInNanoSeconds),
      dispatch_get_main_queue(), ^{
        [weakSelf.mediator setState:PaymentRequestSelectorStatePending];
        [weakSelf.viewController loadModel];
        [weakSelf.viewController.collectionView reloadData];

        [weakSelf.delegate shippingOptionSelectionCoordinator:weakSelf
                                      didSelectShippingOption:shippingOption];
      });
}

@end

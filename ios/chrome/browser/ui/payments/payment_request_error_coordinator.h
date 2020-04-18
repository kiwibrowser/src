// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_ERROR_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_ERROR_COORDINATOR_H_

#import <UIKit/UIKit.h>

#include "base/ios/block_types.h"
#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"
#import "ios/chrome/browser/ui/payments/payment_request_error_view_controller.h"

@class PaymentRequestErrorCoordinator;

// Delegate protocol for PaymentRequestErrorCoordinator.
@protocol PaymentRequestErrorCoordinatorDelegate<NSObject>

// Notifies the delegate that the user has dismissed the error.
- (void)paymentRequestErrorCoordinatorDidDismiss:
    (PaymentRequestErrorCoordinator*)coordinator;

@end

// Coordinator responsible for creating and presenting the payment request error
// view controller.
@interface PaymentRequestErrorCoordinator
    : ChromeCoordinator<PaymentRequestErrorViewControllerDelegate>

// The callback to be called once the error is dismissed, if any.
@property(nonatomic, copy) ProceduralBlock callback;

// The delegate to be notified when the user dismisses the error.
@property(nonatomic, weak) id<PaymentRequestErrorCoordinatorDelegate> delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_ERROR_COORDINATOR_H_

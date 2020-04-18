// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_ERROR_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_ERROR_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/collection_view_controller.h"

extern NSString* const kPaymentRequestErrorCollectionViewID;

@class PaymentRequestErrorViewController;

// Delegate protocol for PaymentRequestErrorViewController.
@protocol PaymentRequestErrorViewControllerDelegate<NSObject>

// Notifies the delegate that the user has dismissed the error.
- (void)paymentRequestErrorViewControllerDidDismiss:
    (PaymentRequestErrorViewController*)controller;

@end

// View controller responsible for presenting the error for the payment request.
@interface PaymentRequestErrorViewController : CollectionViewController

// The error message to display, if any.
@property(nonatomic, copy) NSString* errorMessage;

// The delegate to be notified when the user dismisses the error.
@property(nonatomic, weak) id<PaymentRequestErrorViewControllerDelegate>
    delegate;

- (instancetype)init NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style
    NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_ERROR_VIEW_CONTROLLER_H_

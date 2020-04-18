// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_EDIT_VIEW_CONTROLLER_VALIDATOR_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_EDIT_VIEW_CONTROLLER_VALIDATOR_H_

#import <Foundation/Foundation.h>

@class PaymentRequestEditViewController;
@class EditorField;

// Validator protocol for PaymentRequestEditViewController.
@protocol PaymentRequestEditViewControllerValidator<NSObject>

// Returns the validation error string for |field|. Returns nil if there are no
// validation errors.
- (NSString*)paymentRequestEditViewController:
                 (PaymentRequestEditViewController*)controller
                                validateField:(EditorField*)field;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_EDIT_VIEW_CONTROLLER_VALIDATOR_H_

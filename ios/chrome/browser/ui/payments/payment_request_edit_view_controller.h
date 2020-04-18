// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_EDIT_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_EDIT_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>
#include <vector>

#import "ios/chrome/browser/ui/autofill/autofill_ui_type.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller.h"
#import "ios/chrome/browser/ui/payments/payment_request_edit_consumer.h"
#import "ios/chrome/browser/ui/payments/payment_request_edit_view_controller_data_source.h"
#import "ios/chrome/browser/ui/payments/payment_request_edit_view_controller_validator.h"

extern NSString* const kWarningMessageAccessibilityID;

@class EditorField;
@class PaymentRequestEditViewController;

// Delegate protocol for PaymentRequestEditViewController.
@protocol PaymentRequestEditViewControllerDelegate<NSObject>

// Notifies the delegate that the user has finished editing the editor fields.
- (void)paymentRequestEditViewController:
            (PaymentRequestEditViewController*)controller
                  didFinishEditingFields:(NSArray<EditorField*>*)fields;

// Notifies the delegate that the user has chosen to discard entries in the
// editor fields and return to the previous screen.
- (void)paymentRequestEditViewControllerDidCancel:
    (PaymentRequestEditViewController*)controller;

@optional

// Notifies the delegate that the user has selected |field|.
- (void)paymentRequestEditViewController:
            (PaymentRequestEditViewController*)controller
                          didSelectField:(EditorField*)field;

@end

// The collection view controller for a generic Payment Request edit screen. It
// features sections for every EditorField supplied to the initializer. Each
// section has a text field as well as an error message item which is visible
// when the value of its respective text field is invalid.
@interface PaymentRequestEditViewController
    : CollectionViewController<PaymentRequestEditConsumer>

// The data source for this view controller.
@property(nonatomic, weak) id<PaymentRequestEditViewControllerDataSource>
    dataSource;

// The delegate to be notified when the user selects an editor field.
@property(nonatomic, weak) id<PaymentRequestEditViewControllerDelegate>
    delegate;

// The delegate to be called for validating the fields. By default, the
// controller is the validator.
@property(nonatomic, weak) id<PaymentRequestEditViewControllerValidator>
    validatorDelegate;

// Convenience initializer. Initializes this view controller with the
// CollectionViewControllerStyleAppBar style and sets up the leading (cancel)
// and the trailing (save) buttons.
- (instancetype)init;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_EDIT_VIEW_CONTROLLER_H_

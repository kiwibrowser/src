// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/collection_view_controller.h"
#import "ios/chrome/browser/ui/payments/payment_request_view_controller_data_source.h"

extern NSString* const kPaymentRequestCollectionViewID;

@class PaymentRequestViewController;

// Delegate protocol for PaymentRequestViewController.
@protocol PaymentRequestViewControllerDelegate<NSObject>

// Notifies the delegate that the user has canceled the payment request.
- (void)paymentRequestViewControllerDidCancel:
    (PaymentRequestViewController*)controller;

// Notifies the delegate that the user has confirmed the payment request.
- (void)paymentRequestViewControllerDidConfirm:
    (PaymentRequestViewController*)controller;

// Notifies the delegate that the user has selected to go to the card and
// address options page in Settings.
- (void)paymentRequestViewControllerDidSelectSettings:
    (PaymentRequestViewController*)controller;

// Notifies the delegate that the user has selected the payment summary item.
- (void)paymentRequestViewControllerDidSelectPaymentSummaryItem:
    (PaymentRequestViewController*)controller;

// Notifies the delegate that the user has selected the contact info item.
- (void)paymentRequestViewControllerDidSelectContactInfoItem:
    (PaymentRequestViewController*)controller;

// Notifies the delegate that the user has selected the shipping address item.
- (void)paymentRequestViewControllerDidSelectShippingAddressItem:
    (PaymentRequestViewController*)controller;

// Notifies the delegate that the user has selected the shipping option item.
- (void)paymentRequestViewControllerDidSelectShippingOptionItem:
    (PaymentRequestViewController*)controller;

// Notifies the delegate that the user has selected the payment method item.
- (void)paymentRequestViewControllerDidSelectPaymentMethodItem:
    (PaymentRequestViewController*)controller;

@end

// View controller responsible for presenting the details of a PaymentRequest to
// the user and communicating their choices to the supplied delegate.
@interface PaymentRequestViewController : CollectionViewController

// The favicon of the page invoking the Payment Request API.
@property(nonatomic, strong) UIImage* pageFavicon;

// The title of the page invoking the Payment Request API.
@property(nonatomic, copy) NSString* pageTitle;

// The host of the page invoking the Payment Request API.
@property(nonatomic, copy) NSString* pageHost;

// Whether or not the connection is secure.
@property(nonatomic, assign, getter=isConnectionSecure) BOOL connectionSecure;

// Whether or not the view is in a pending state.
@property(nonatomic, assign, getter=isPending) BOOL pending;

// Whether or not the user can cancel out of the view.
@property(nonatomic, assign, getter=isCancellable) BOOL cancellable;

// The delegate to be notified when the user confirms or cancels the request.
@property(nonatomic, weak) id<PaymentRequestViewControllerDelegate> delegate;

// The data source for this view controller.
@property(nonatomic, weak) id<PaymentRequestViewControllerDataSource>
    dataSource;

// Updates the payment summary item in the summary section.
- (void)updatePaymentSummaryItem;

// Reloads the shipping, payment method and contact info sections.
- (void)reloadSections;

// Reloads the shipping section.
- (void)reloadShippingSection;

// Reloads the payment method section.
- (void)reloadPaymentMethodSection;

// Reloads the contact info section.
- (void)reloadContactInfoSection;

- (instancetype)init NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style
    NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_VIEW_CONTROLLER_H_

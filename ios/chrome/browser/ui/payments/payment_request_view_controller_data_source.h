// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_VIEW_CONTROLLER_DATA_SOURCE_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_VIEW_CONTROLLER_DATA_SOURCE_H_

#import <Foundation/Foundation.h>

@class CollectionViewFooterItem;
@class CollectionViewItem;
@class PaymentsTextItem;

// Data source protocol for PaymentRequestViewController.
@protocol PaymentRequestViewControllerDataSource

// Returns whether the payment can be made and therefore the pay button should
// be enabled.
- (BOOL)canPay;

// Returns whether the total price is itemized.
- (BOOL)hasPaymentItems;

// Returns whether shipping is requested and therefore the Shipping section
// should be presented.
- (BOOL)requestShipping;

// Returns whether contact information is requested and therefore the Contact
// Info section should be presented.
- (BOOL)requestContactInfo;

// Returns the Payment Summary item displayed in the Summary section.
- (CollectionViewItem*)paymentSummaryItem;

// Returns the header item for the Shipping section.
- (PaymentsTextItem*)shippingSectionHeaderItem;

// Returns the Shipping Address item displayed in the Shipping section.
- (CollectionViewItem*)shippingAddressItem;

// Returns the Shipping Option item displayed in the Shipping section.
- (CollectionViewItem*)shippingOptionItem;

// Returns the header item for the Payment Method section.
- (PaymentsTextItem*)paymentMethodSectionHeaderItem;

// Returns the item displayed in the Payment Method section.
- (CollectionViewItem*)paymentMethodItem;

// Returns the header item for the Contact Info section.
- (PaymentsTextItem*)contactInfoSectionHeaderItem;

// Returns the item displayed in the Contact Info section.
- (CollectionViewItem*)contactInfoItem;

// Returns the item displayed at the bottom of the view.
- (CollectionViewFooterItem*)footerItem;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_VIEW_CONTROLLER_DATA_SOURCE_H_

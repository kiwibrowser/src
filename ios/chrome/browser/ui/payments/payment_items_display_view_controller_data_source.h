// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_ITEMS_DISPLAY_VIEW_CONTROLLER_DATA_SOURCE_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_ITEMS_DISPLAY_VIEW_CONTROLLER_DATA_SOURCE_H_

#import <Foundation/Foundation.h>

@class CollectionViewItem;

// The data source for the PaymentItemsDisplayViewController. The data source
// provides the UI models for the PaymentItemsDisplayViewController.
@protocol PaymentItemsDisplayViewControllerDataSource<NSObject>

// Returns whether the payment can be made and therefore the pay button should
// be enabled.
- (BOOL)canPay;

// The total price item.
- (CollectionViewItem*)totalItem;

// The line items for the total price.
- (NSArray<CollectionViewItem*>*)lineItems;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_ITEMS_DISPLAY_VIEW_CONTROLLER_DATA_SOURCE_H_

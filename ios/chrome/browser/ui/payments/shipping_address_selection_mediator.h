// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_SHIPPING_ADDRESS_SELECTION_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_SHIPPING_ADDRESS_SELECTION_MEDIATOR_H_

#import "ios/chrome/browser/ui/payments/payment_request_selector_view_controller_data_source.h"

namespace payments {
class PaymentRequest;
}  // namespace payments

// Serves as data source for PaymentRequestSelectorViewController.
@interface ShippingAddressSelectionMediator
    : NSObject<PaymentRequestSelectorViewControllerDataSource>

// Redefined to be read-write.
@property(nonatomic, readwrite, assign) PaymentRequestSelectorState state;

// Redefined to be read-write.
@property(nonatomic, readwrite, assign) NSUInteger selectedItemIndex;

// Creates and stores the selectable items to display in the collection.
- (void)loadItems;

// Initializes this object with an instance of PaymentRequest which has a copy
// of payments::WebPaymentRequest as provided by the page invoking the Payment
// Request API. This object will not take ownership of |paymentRequest|.
- (instancetype)initWithPaymentRequest:(payments::PaymentRequest*)paymentRequest
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_SHIPPING_ADDRESS_SELECTION_MEDIATOR_H_

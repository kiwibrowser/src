// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_SHIPPING_ADDRESS_SELECTION_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_SHIPPING_ADDRESS_SELECTION_COORDINATOR_H_

#import <UIKit/UIKit.h>
#include <vector>

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"
#import "ios/chrome/browser/ui/payments/address_edit_coordinator.h"
#import "ios/chrome/browser/ui/payments/payment_request_selector_view_controller.h"

namespace autofill {
class AutofillProfile;
}  // namespace autofill

namespace payments {
class PaymentRequest;
}  // namespace payments

@class ShippingAddressSelectionCoordinator;

// Delegate protocol for ShippingAddressSelectionCoordinator.
@protocol ShippingAddressSelectionCoordinatorDelegate<NSObject>

// Notifies the delegate that the user has selected a shipping address.
- (void)shippingAddressSelectionCoordinator:
            (ShippingAddressSelectionCoordinator*)coordinator
                   didSelectShippingAddress:
                       (autofill::AutofillProfile*)shippingAddress;

// Notifies the delegate that the user has chosen to return to the previous
// screen without making a selection.
- (void)shippingAddressSelectionCoordinatorDidReturn:
    (ShippingAddressSelectionCoordinator*)coordinator;

@end

// Coordinator responsible for creating and presenting the shipping address
// selection view controller. This view controller will be presented by the view
// controller provided in the initializer.
@interface ShippingAddressSelectionCoordinator
    : ChromeCoordinator<PaymentRequestSelectorViewControllerDelegate,
                        AddressEditCoordinatorDelegate>

// The PaymentRequest object having a copy of payments::WebPaymentRequest as
// provided by the page invoking the Payment Request API. This pointer is not
// owned by this class and should outlive it.
@property(nonatomic, assign) payments::PaymentRequest* paymentRequest;

// The delegate to be notified when the user selects a shipping address or
// returns without selecting one.
@property(nonatomic, weak) id<ShippingAddressSelectionCoordinatorDelegate>
    delegate;

// Stops the spinner and displays the error provided in the payment details.
- (void)stopSpinnerAndDisplayError;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_SHIPPING_ADDRESS_SELECTION_COORDINATOR_H_

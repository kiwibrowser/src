// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_BILLING_ADDRESS_SELECTION_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_BILLING_ADDRESS_SELECTION_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"
#import "ios/chrome/browser/ui/payments/address_edit_coordinator.h"
#import "ios/chrome/browser/ui/payments/payment_request_selector_view_controller.h"

namespace autofill {
class AutofillProfile;
}  // namespace autofill

namespace payments {
class PaymentRequest;
}  // namespace payments

@class BillingAddressSelectionCoordinator;

// Delegate protocol for BillingAddressSelectionCoordinator.
@protocol BillingAddressSelectionCoordinatorDelegate<NSObject>

// Notifies the delegate that the user has selected a billing address.
- (void)billingAddressSelectionCoordinator:
            (BillingAddressSelectionCoordinator*)coordinator
                   didSelectBillingAddress:
                       (autofill::AutofillProfile*)billingAddress;

// Notifies the delegate that the user has chosen to return to the previous
// screen without making a selection.
- (void)billingAddressSelectionCoordinatorDidReturn:
    (BillingAddressSelectionCoordinator*)coordinator;

@end

// Coordinator responsible for creating and presenting the billing address
// selection view controller. This view controller will be presented by the view
// controller provided in the initializer.
@interface BillingAddressSelectionCoordinator
    : ChromeCoordinator<PaymentRequestSelectorViewControllerDelegate,
                        AddressEditCoordinatorDelegate>

// The selected billing profile, if any.
@property(nonatomic, assign) autofill::AutofillProfile* selectedBillingProfile;

// The PaymentRequest object having a copy of payments::WebPaymentRequest as
// provided by the page invoking the Payment Request API. This pointer is not
// owned by this class and should outlive it.
@property(nonatomic, assign) payments::PaymentRequest* paymentRequest;

// The delegate to be notified when the user selects a billing address or
// returns without selecting one.
@property(nonatomic, weak) id<BillingAddressSelectionCoordinatorDelegate>
    delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_BILLING_ADDRESS_SELECTION_COORDINATOR_H_

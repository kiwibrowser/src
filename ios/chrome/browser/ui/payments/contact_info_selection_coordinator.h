// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_CONTACT_INFO_SELECTION_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_CONTACT_INFO_SELECTION_COORDINATOR_H_

#import <UIKit/UIKit.h>
#include <vector>

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"
#import "ios/chrome/browser/ui/payments/contact_info_edit_coordinator.h"
#import "ios/chrome/browser/ui/payments/payment_request_selector_view_controller.h"

namespace autofill {
class AutofillProfile;
}  // namespace autofill

namespace payments {
class PaymentRequest;
}  // namespace payments

@class ContactInfoSelectionCoordinator;

// Delegate protocol for ContactInfoSelectionCoordinator.
@protocol ContactInfoSelectionCoordinatorDelegate<NSObject>

// Notifies the delegate that the user has selected a contact profile.
- (void)
contactInfoSelectionCoordinator:(ContactInfoSelectionCoordinator*)coordinator
        didSelectContactProfile:(autofill::AutofillProfile*)contactProfile;

// Notifies the delegate that the user has chosen to return to the previous
// screen without making a selection.
- (void)contactInfoSelectionCoordinatorDidReturn:
    (ContactInfoSelectionCoordinator*)coordinator;

@end

// Coordinator responsible for creating and presenting the contact info
// selection view controller. This view controller will be presented by the view
// controller provided in the initializer.
@interface ContactInfoSelectionCoordinator
    : ChromeCoordinator<PaymentRequestSelectorViewControllerDelegate,
                        ContactInfoEditCoordinatorDelegate>

// The PaymentRequest object having a copy of payments::WebPaymentRequest as
// provided by the page invoking the Payment Request API. This pointer is not
// owned by this class and should outlive it.
@property(nonatomic, assign) payments::PaymentRequest* paymentRequest;

// The delegate to be notified when the user selects a contact profile or
// returns without selecting one.
@property(nonatomic, weak) id<ContactInfoSelectionCoordinatorDelegate> delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_CONTACT_INFO_SELECTION_COORDINATOR_H_

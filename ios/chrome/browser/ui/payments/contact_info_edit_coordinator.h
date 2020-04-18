// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_CONTACT_INFO_EDIT_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_CONTACT_INFO_EDIT_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"
#import "ios/chrome/browser/ui/payments/payment_request_edit_view_controller.h"

namespace autofill {
class AutofillProfile;
}  // namespace autofill

namespace payments {
class PaymentRequest;
}  // namespace payments

@class ContactInfoEditCoordinator;

// Delegate protocol for ContactInfoEditCoordinator.
@protocol ContactInfoEditCoordinatorDelegate<NSObject>

// Notifies the delegate that the user has finished editing or creating
// |profile|. |profile| will be a new autofill profile instance owned by the
// PaymentRequest object if none was provided to the coordinator. Otherwise, it
// will be the same edited instance.
- (void)contactInfoEditCoordinator:(ContactInfoEditCoordinator*)coordinator
           didFinishEditingProfile:(autofill::AutofillProfile*)profile;

// Notifies the delegate that the user has chosen to cancel editing or creating
// a profile and return to the previous screen.
- (void)contactInfoEditCoordinatorDidCancel:
    (ContactInfoEditCoordinator*)coordinator;

@end

// Coordinator responsible for creating and presenting a profile editor view
// controller. This view controller will be presented by the view controller
// provided in the initializer.
@interface ContactInfoEditCoordinator
    : ChromeCoordinator<PaymentRequestEditViewControllerDelegate>

// The profile to be edited, if any. This pointer is not owned by this class
// and should outlive it.
@property(nonatomic, assign) autofill::AutofillProfile* profile;

// The PaymentRequest object owning an instance of payments::WebPaymentRequest
// as provided by the page invoking the Payment Request API. This pointer is not
// owned by this class and should outlive it.
@property(nonatomic, assign) payments::PaymentRequest* paymentRequest;

// The delegate to be notified when the user returns or finishes creating or
// editing a profile.
@property(nonatomic, weak) id<ContactInfoEditCoordinatorDelegate> delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_CONTACT_INFO_EDIT_COORDINATOR_H_

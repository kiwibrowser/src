// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_CREDIT_CARD_EDIT_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_CREDIT_CARD_EDIT_MEDIATOR_H_

#import "ios/chrome/browser/ui/payments/payment_request_edit_view_controller_data_source.h"
#import "ios/chrome/browser/ui/payments/payment_request_edit_view_controller_validator.h"

@protocol PaymentRequestEditConsumer;

namespace autofill {
class AutofillProfile;
class CreditCard;
}  // namespace autofill

namespace payments {
class PaymentRequest;
}  // namespace payments

// Serves as data source for CreditCardEditViewController.
@interface CreditCardEditMediator
    : NSObject<PaymentRequestEditViewControllerDataSource,
               PaymentRequestEditViewControllerValidator>

// The consumer for this object. This can change during the lifetime of this
// object and may be nil.
@property(nonatomic, weak) id<PaymentRequestEditConsumer> consumer;

// The billing profile, if any.
@property(nonatomic, assign) autofill::AutofillProfile* billingProfile;

// Initializes this object with an instance of PaymentRequest which has a copy
// of payments::WebPaymentRequest as provided by the page invoking the Payment
// Request API as well as |creditCard| which is the credit card to be edited, if
// any. This object will not take ownership of |paymentRequest| or |creditCard|.
- (instancetype)initWithPaymentRequest:(payments::PaymentRequest*)paymentRequest
                            creditCard:(autofill::CreditCard*)creditCard
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_CREDIT_CARD_EDIT_MEDIATOR_H_

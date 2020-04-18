// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_CREDIT_CARD_INTERNAL_H_
#define IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_CREDIT_CARD_INTERNAL_H_

#import "ios/web_view/public/cwv_credit_card.h"

namespace autofill {
class CreditCard;
}  // namespace autofill

@interface CWVCreditCard ()

// The internal autofill credit card that is wrapped by this object.
@property(nonatomic, readonly) autofill::CreditCard* internalCard;

- (instancetype)initWithCreditCard:(const autofill::CreditCard&)creditCard
    NS_DESIGNATED_INITIALIZER;

@end

#endif  // IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_CREDIT_CARD_INTERNAL_H_

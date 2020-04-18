// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_PAYMENT_DETAILS_MODIFIER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_PAYMENT_DETAILS_MODIFIER_H_

#include "third_party/blink/public/platform/modules/payments/web_payment_item.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace blink {

// https://w3c.github.io/browser-payment-api/#paymentdetailsmodifier-dictionary
struct WebPaymentDetailsModifier {
  WebVector<WebString> supported_methods;
  WebPaymentItem total;
  WebVector<WebPaymentItem> additional_display_items;
  WebString stringified_data;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_PAYMENT_DETAILS_MODIFIER_H_

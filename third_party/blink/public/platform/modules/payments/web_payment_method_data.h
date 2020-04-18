// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_PAYMENT_METHOD_DATA_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_PAYMENT_METHOD_DATA_H_

#include "third_party/blink/public/platform/modules/payments/web_payment_details_modifier.h"
#include "third_party/blink/public/platform/modules/payments/web_payment_item.h"
#include "third_party/blink/public/platform/modules/payments/web_payment_method_data.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace blink {

// https://w3c.github.io/browser-payment-api/#paymentmethoddata-dictionary
struct WebPaymentMethodData {
  WebVector<WebString> supported_methods;
  WebString stringified_data;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_PAYMENT_METHOD_DATA_H_

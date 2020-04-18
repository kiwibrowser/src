// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_PAYMENT_REQUEST_EVENT_DATA_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_PAYMENT_REQUEST_EVENT_DATA_H_

#include "third_party/blink/public/platform/modules/payments/web_can_make_payment_event_data.h"
#include "third_party/blink/public/platform/modules/payments/web_payment_currency_amount.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {

struct WebPaymentRequestEventData : public WebCanMakePaymentEventData {
  WebString payment_request_id;
  WebString instrument_key;
  WebPaymentCurrencyAmount total;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_PAYMENT_REQUEST_EVENT_DATA_H_

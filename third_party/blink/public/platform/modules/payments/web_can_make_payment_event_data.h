// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_CAN_MAKE_PAYMENT_EVENT_DATA_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_CAN_MAKE_PAYMENT_EVENT_DATA_H_

#include "third_party/blink/public/platform/modules/payments/web_payment_details_modifier.h"
#include "third_party/blink/public/platform/modules/payments/web_payment_item.h"
#include "third_party/blink/public/platform/modules/payments/web_payment_method_data.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace blink {

struct WebCanMakePaymentEventData {
  WebString top_origin;
  WebString payment_request_origin;
  WebVector<WebPaymentMethodData> method_data;
  WebVector<WebPaymentDetailsModifier> modifiers;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_CAN_MAKE_PAYMENT_EVENT_DATA_H_

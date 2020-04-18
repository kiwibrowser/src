// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_TYPE_CONVERTERS_H_
#define CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_TYPE_CONVERTERS_H_

#include "content/common/service_worker/service_worker_event_dispatcher.mojom.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_event_status.mojom.h"
#include "third_party/blink/public/platform/modules/payments/payment_app.mojom.h"
#include "third_party/blink/public/platform/modules/payments/web_can_make_payment_event_data.h"
#include "third_party/blink/public/platform/modules/payments/web_payment_request_event_data.h"
#include "third_party/blink/public/web/modules/serviceworker/web_service_worker_context_proxy.h"

namespace mojo {

template <>
struct TypeConverter<blink::WebCanMakePaymentEventData,
                     payments::mojom::CanMakePaymentEventDataPtr> {
  static blink::WebCanMakePaymentEventData Convert(
      const payments::mojom::CanMakePaymentEventDataPtr& input);
};

template <>
struct TypeConverter<blink::WebPaymentRequestEventData,
                     payments::mojom::PaymentRequestEventDataPtr> {
  static blink::WebPaymentRequestEventData Convert(
      const payments::mojom::PaymentRequestEventDataPtr& input);
};

template <>
struct TypeConverter<blink::WebPaymentMethodData,
                     payments::mojom::PaymentMethodDataPtr> {
  static blink::WebPaymentMethodData Convert(
      const payments::mojom::PaymentMethodDataPtr& input);
};

template <>
struct TypeConverter<blink::WebPaymentItem, payments::mojom::PaymentItemPtr> {
  static blink::WebPaymentItem Convert(
      const payments::mojom::PaymentItemPtr& input);
};

template <>
struct TypeConverter<blink::WebPaymentCurrencyAmount,
                     payments::mojom::PaymentCurrencyAmountPtr> {
  static blink::WebPaymentCurrencyAmount Convert(
      const payments::mojom::PaymentCurrencyAmountPtr& input);
};

template <>
struct TypeConverter<blink::WebPaymentDetailsModifier,
                     payments::mojom::PaymentDetailsModifierPtr> {
  static blink::WebPaymentDetailsModifier Convert(
      const payments::mojom::PaymentDetailsModifierPtr& input);
};

template <>
struct TypeConverter<blink::WebServiceWorkerContextProxy::BackgroundFetchState,
                     content::mojom::BackgroundFetchState> {
  static blink::WebServiceWorkerContextProxy::BackgroundFetchState Convert(
      content::mojom::BackgroundFetchState input);
};

}  // namespace

#endif  // CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_TYPE_CONVERTERS_H_

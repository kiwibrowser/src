// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/payments/payment_request_respond_with_observer.h"

#include <v8.h>
#include "third_party/blink/public/platform/modules/payments/web_payment_handler_response.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_payment_handler_response.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/modules/payments/payment_handler_response.h"
#include "third_party/blink/renderer/modules/payments/payment_handler_utils.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_global_scope_client.h"
#include "third_party/blink/renderer/modules/serviceworkers/wait_until_observer.h"

namespace blink {

PaymentRequestRespondWithObserver* PaymentRequestRespondWithObserver::Create(
    ExecutionContext* context,
    int event_id,
    WaitUntilObserver* observer) {
  return new PaymentRequestRespondWithObserver(context, event_id, observer);
}

void PaymentRequestRespondWithObserver::OnResponseRejected(
    mojom::ServiceWorkerResponseError error) {
  PaymentHandlerUtils::ReportResponseError(GetExecutionContext(),
                                           "PaymentRequestEvent", error);

  WebPaymentHandlerResponse web_data;
  ServiceWorkerGlobalScopeClient::From(GetExecutionContext())
      ->RespondToPaymentRequestEvent(event_id_, web_data, event_dispatch_time_);
}

void PaymentRequestRespondWithObserver::OnResponseFulfilled(
    const ScriptValue& value) {
  DCHECK(GetExecutionContext());
  ExceptionState exception_state(value.GetIsolate(),
                                 ExceptionState::kUnknownContext,
                                 "PaymentRequestEvent", "respondWith");
  PaymentHandlerResponse response = ScriptValue::To<PaymentHandlerResponse>(
      ToIsolate(GetExecutionContext()), value, exception_state);
  if (exception_state.HadException()) {
    exception_state.ClearException();
    OnResponseRejected(mojom::ServiceWorkerResponseError::kNoV8Instance);
    return;
  }

  // Check payment response validity.
  if (!response.hasMethodName() || !response.hasDetails()) {
    GetExecutionContext()->AddConsoleMessage(
        ConsoleMessage::Create(kJSMessageSource, kErrorMessageLevel,
                               "'PaymentHandlerResponse.methodName' and "
                               "'PaymentHandlerResponse.details' must not "
                               "be empty in payment response."));
    OnResponseRejected(mojom::ServiceWorkerResponseError::kUnknown);
    return;
  }

  WebPaymentHandlerResponse web_data;
  web_data.method_name = response.methodName();

  v8::Local<v8::String> details_value;
  if (!v8::JSON::Stringify(response.details().GetContext(),
                           response.details().V8Value().As<v8::Object>())
           .ToLocal(&details_value)) {
    GetExecutionContext()->AddConsoleMessage(ConsoleMessage::Create(
        kJSMessageSource, kErrorMessageLevel,
        "Failed to stringify PaymentHandlerResponse.details in payment "
        "response."));
    OnResponseRejected(mojom::ServiceWorkerResponseError::kUnknown);
    return;
  }
  web_data.stringified_details = ToCoreString(details_value);
  ServiceWorkerGlobalScopeClient::From(GetExecutionContext())
      ->RespondToPaymentRequestEvent(event_id_, web_data, event_dispatch_time_);
}

void PaymentRequestRespondWithObserver::OnNoResponse() {
  DCHECK(GetExecutionContext());
  ServiceWorkerGlobalScopeClient::From(GetExecutionContext())
      ->RespondToPaymentRequestEvent(event_id_, WebPaymentHandlerResponse(),
                                     event_dispatch_time_);
}

PaymentRequestRespondWithObserver::PaymentRequestRespondWithObserver(
    ExecutionContext* context,
    int event_id,
    WaitUntilObserver* observer)
    : RespondWithObserver(context, event_id, observer) {}

void PaymentRequestRespondWithObserver::Trace(blink::Visitor* visitor) {
  RespondWithObserver::Trace(visitor);
}

}  // namespace blink

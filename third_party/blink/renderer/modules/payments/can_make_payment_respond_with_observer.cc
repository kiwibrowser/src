// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/payments/can_make_payment_respond_with_observer.h"

#include <v8.h>
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/modules/payments/payment_handler_utils.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_global_scope_client.h"
#include "third_party/blink/renderer/modules/serviceworkers/wait_until_observer.h"

namespace blink {

CanMakePaymentRespondWithObserver::CanMakePaymentRespondWithObserver(
    ExecutionContext* context,
    int event_id,
    WaitUntilObserver* observer)
    : RespondWithObserver(context, event_id, observer) {}

void CanMakePaymentRespondWithObserver::OnResponseRejected(
    blink::mojom::ServiceWorkerResponseError error) {
  PaymentHandlerUtils::ReportResponseError(GetExecutionContext(),
                                           "CanMakePaymentEvent", error);

  ServiceWorkerGlobalScopeClient::From(GetExecutionContext())
      ->RespondToCanMakePaymentEvent(event_id_, false, event_dispatch_time_);
}

void CanMakePaymentRespondWithObserver::OnResponseFulfilled(
    const ScriptValue& value) {
  DCHECK(GetExecutionContext());
  ExceptionState exception_state(value.GetIsolate(),
                                 ExceptionState::kUnknownContext,
                                 "PaymentRequestEvent", "respondWith");
  bool response = ToBoolean(ToIsolate(GetExecutionContext()), value.V8Value(),
                            exception_state);
  if (exception_state.HadException()) {
    exception_state.ClearException();
    OnResponseRejected(blink::mojom::ServiceWorkerResponseError::kNoV8Instance);
    return;
  }

  ServiceWorkerGlobalScopeClient::From(GetExecutionContext())
      ->RespondToCanMakePaymentEvent(event_id_, response, event_dispatch_time_);
}

void CanMakePaymentRespondWithObserver::OnNoResponse() {
  DCHECK(GetExecutionContext());
  ServiceWorkerGlobalScopeClient::From(GetExecutionContext())
      ->RespondToCanMakePaymentEvent(event_id_, true, event_dispatch_time_);
}

void CanMakePaymentRespondWithObserver::Trace(blink::Visitor* visitor) {
  RespondWithObserver::Trace(visitor);
}

}  // namespace blink

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/payments/payment_event_data_conversion.h"

#include "third_party/blink/public/platform/modules/payments/web_payment_method_data.h"
#include "third_party/blink/public/platform/modules/payments/web_payment_request_event_data.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_for_core.h"
#include "third_party/blink/renderer/modules/payments/payment_currency_amount.h"
#include "third_party/blink/renderer/modules/payments/payment_details_modifier.h"
#include "third_party/blink/renderer/modules/payments/payment_item.h"
#include "third_party/blink/renderer/modules/payments/payment_method_data.h"
#include "third_party/blink/renderer/modules/payments/payment_request_event_init.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {
namespace {

PaymentCurrencyAmount ToPaymentCurrencyAmount(
    const WebPaymentCurrencyAmount& web_amount) {
  PaymentCurrencyAmount amount;
  amount.setCurrency(web_amount.currency);
  amount.setValue(web_amount.value);
  return amount;
}

PaymentItem ToPaymentItem(const WebPaymentItem& web_item) {
  PaymentItem item;
  item.setLabel(web_item.label);
  item.setAmount(ToPaymentCurrencyAmount(web_item.amount));
  item.setPending(web_item.pending);
  return item;
}

PaymentDetailsModifier ToPaymentDetailsModifier(
    ScriptState* script_state,
    const WebPaymentDetailsModifier& web_modifier) {
  PaymentDetailsModifier modifier;
  Vector<String> supported_methods;
  for (const auto& web_method : web_modifier.supported_methods) {
    supported_methods.push_back(web_method);
  }
  modifier.setSupportedMethods(
      StringOrStringSequence::FromStringSequence(supported_methods));
  modifier.setTotal(ToPaymentItem(web_modifier.total));
  HeapVector<PaymentItem> additional_display_items;
  for (const auto& web_item : web_modifier.additional_display_items) {
    additional_display_items.push_back(ToPaymentItem(web_item));
  }
  modifier.setAdditionalDisplayItems(additional_display_items);
  return modifier;
}

ScriptValue StringDataToScriptValue(ScriptState* script_state,
                                    const WebString& stringified_data) {
  if (!script_state->ContextIsValid())
    return ScriptValue();

  ScriptState::Scope scope(script_state);
  v8::Local<v8::Value> v8_value;
  if (!v8::JSON::Parse(script_state->GetContext(),
                       V8String(script_state->GetIsolate(), stringified_data))
           .ToLocal(&v8_value)) {
    return ScriptValue();
  }
  return ScriptValue(script_state, v8_value);
}

PaymentMethodData ToPaymentMethodData(
    ScriptState* script_state,
    const WebPaymentMethodData& web_method_data) {
  PaymentMethodData method_data;
  Vector<String> supported_methods;
  for (const auto& method : web_method_data.supported_methods) {
    supported_methods.push_back(method);
  }
  method_data.setSupportedMethods(
      StringOrStringSequence::FromStringSequence(supported_methods));
  method_data.setData(
      StringDataToScriptValue(script_state, web_method_data.stringified_data));
  return method_data;
}

}  // namespace

PaymentRequestEventInit PaymentEventDataConversion::ToPaymentRequestEventInit(
    ScriptState* script_state,
    const WebPaymentRequestEventData& web_event_data) {
  DCHECK(script_state);

  PaymentRequestEventInit event_data;
  if (!script_state->ContextIsValid())
    return event_data;

  ScriptState::Scope scope(script_state);

  event_data.setTopOrigin(web_event_data.top_origin);
  event_data.setPaymentRequestOrigin(web_event_data.payment_request_origin);
  event_data.setPaymentRequestId(web_event_data.payment_request_id);
  HeapVector<PaymentMethodData> method_data;
  for (const auto& md : web_event_data.method_data) {
    method_data.push_back(ToPaymentMethodData(script_state, md));
  }
  event_data.setMethodData(method_data);
  event_data.setTotal(ToPaymentCurrencyAmount(web_event_data.total));
  HeapVector<PaymentDetailsModifier> modifiers;
  for (const auto& modifier : web_event_data.modifiers) {
    modifiers.push_back(ToPaymentDetailsModifier(script_state, modifier));
  }
  event_data.setModifiers(modifiers);
  event_data.setInstrumentKey(web_event_data.instrument_key);
  return event_data;
}

CanMakePaymentEventInit PaymentEventDataConversion::ToCanMakePaymentEventInit(
    ScriptState* script_state,
    const WebCanMakePaymentEventData& web_event_data) {
  DCHECK(script_state);

  CanMakePaymentEventInit event_data;
  if (!script_state->ContextIsValid())
    return event_data;

  ScriptState::Scope scope(script_state);

  event_data.setTopOrigin(web_event_data.top_origin);
  event_data.setPaymentRequestOrigin(web_event_data.payment_request_origin);
  HeapVector<PaymentMethodData> method_data;
  for (const auto& md : web_event_data.method_data) {
    method_data.push_back(ToPaymentMethodData(script_state, md));
  }
  event_data.setMethodData(method_data);
  HeapVector<PaymentDetailsModifier> modifiers;
  for (const auto& modifier : web_event_data.modifiers) {
    modifiers.push_back(ToPaymentDetailsModifier(script_state, modifier));
  }
  event_data.setModifiers(modifiers);
  return event_data;
}

}  // namespace blink

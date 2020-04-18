// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_PAYMENT_REQUEST_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_PAYMENT_REQUEST_EVENT_H_

#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/modules/payments/payment_request_event_init.h"
#include "third_party/blink/renderer/modules/serviceworkers/extendable_event.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace WTF {
class AtomicString;
}

namespace blink {

class RespondWithObserver;
class ScriptState;

class MODULES_EXPORT PaymentRequestEvent final : public ExtendableEvent {
  DEFINE_WRAPPERTYPEINFO();
  WTF_MAKE_NONCOPYABLE(PaymentRequestEvent);

 public:
  static PaymentRequestEvent* Create(const AtomicString& type,
                                     const PaymentRequestEventInit&);
  static PaymentRequestEvent* Create(const AtomicString& type,
                                     const PaymentRequestEventInit&,
                                     RespondWithObserver*,
                                     WaitUntilObserver*);
  ~PaymentRequestEvent() override;

  const AtomicString& InterfaceName() const override;

  const String& topOrigin() const;
  const String& paymentRequestOrigin() const;
  const String& paymentRequestId() const;
  const HeapVector<PaymentMethodData>& methodData() const;
  const ScriptValue total(ScriptState*) const;
  const HeapVector<PaymentDetailsModifier>& modifiers() const;
  const String& instrumentKey() const;

  ScriptPromise openWindow(ScriptState*, const String& url);
  void respondWith(ScriptState*, ScriptPromise, ExceptionState&);

  void Trace(blink::Visitor*) override;

 private:
  PaymentRequestEvent(const AtomicString& type,
                      const PaymentRequestEventInit&,
                      RespondWithObserver*,
                      WaitUntilObserver*);

  String top_origin_;
  String payment_request_origin_;
  String payment_request_id_;
  HeapVector<PaymentMethodData> method_data_;
  PaymentCurrencyAmount total_;
  HeapVector<PaymentDetailsModifier> modifiers_;
  String instrument_key_;

  Member<RespondWithObserver> observer_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_PAYMENT_REQUEST_EVENT_H_

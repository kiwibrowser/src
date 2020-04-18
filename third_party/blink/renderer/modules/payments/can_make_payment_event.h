// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_CAN_MAKE_PAYMENT_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_CAN_MAKE_PAYMENT_EVENT_H_

#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/modules/payments/can_make_payment_event_init.h"
#include "third_party/blink/renderer/modules/payments/payment_details_modifier.h"
#include "third_party/blink/renderer/modules/payments/payment_method_data.h"
#include "third_party/blink/renderer/modules/serviceworkers/extendable_event.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace WTF {
class AtomicString;
}

namespace blink {

class RespondWithObserver;
class ScriptState;

class MODULES_EXPORT CanMakePaymentEvent final : public ExtendableEvent {
  DEFINE_WRAPPERTYPEINFO();
  WTF_MAKE_NONCOPYABLE(CanMakePaymentEvent);

 public:
  static CanMakePaymentEvent* Create(const AtomicString& type,
                                     const CanMakePaymentEventInit&);
  static CanMakePaymentEvent* Create(const AtomicString& type,
                                     const CanMakePaymentEventInit&,
                                     RespondWithObserver*,
                                     WaitUntilObserver*);
  ~CanMakePaymentEvent() override;

  const AtomicString& InterfaceName() const override;

  const String& topOrigin() const;
  const String& paymentRequestOrigin() const;
  const HeapVector<PaymentMethodData>& methodData() const;
  const HeapVector<PaymentDetailsModifier>& modifiers() const;

  void respondWith(ScriptState*, ScriptPromise, ExceptionState&);

  void Trace(blink::Visitor*) override;

 private:
  CanMakePaymentEvent(const AtomicString& type,
                      const CanMakePaymentEventInit&,
                      RespondWithObserver*,
                      WaitUntilObserver*);

  String top_origin_;
  String payment_request_origin_;
  HeapVector<PaymentMethodData> method_data_;
  HeapVector<PaymentDetailsModifier> modifiers_;

  Member<RespondWithObserver> observer_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_CAN_MAKE_PAYMENT_EVENT_H_

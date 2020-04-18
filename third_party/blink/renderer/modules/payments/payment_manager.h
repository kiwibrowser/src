// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_PAYMENT_MANAGER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_PAYMENT_MANAGER_H_

#include "third_party/blink/public/platform/modules/payments/payment_app.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class PaymentInstruments;
class ServiceWorkerRegistration;

class MODULES_EXPORT PaymentManager final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();
  WTF_MAKE_NONCOPYABLE(PaymentManager);

 public:
  static PaymentManager* Create(ServiceWorkerRegistration*);

  PaymentInstruments* instruments();

  const String& userHint();
  void setUserHint(const String&);

  void Trace(blink::Visitor*) override;

 private:
  explicit PaymentManager(ServiceWorkerRegistration*);

  void OnServiceConnectionError();

  Member<ServiceWorkerRegistration> registration_;
  payments::mojom::blink::PaymentManagerPtr manager_;
  Member<PaymentInstruments> instruments_;
  String user_hint_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PAYMENTS_PAYMENT_MANAGER_H_

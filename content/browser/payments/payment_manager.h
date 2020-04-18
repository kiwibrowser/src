// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PAYMENTS_PAYMENT_MANAGER_H_
#define CONTENT_BROWSER_PAYMENTS_PAYMENT_MANAGER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/public/platform/modules/payments/payment_app.mojom.h"
#include "url/gurl.h"

namespace content {

class PaymentAppContextImpl;

class CONTENT_EXPORT PaymentManager : public payments::mojom::PaymentManager {
 public:
  PaymentManager(
      PaymentAppContextImpl* payment_app_context,
      mojo::InterfaceRequest<payments::mojom::PaymentManager> request);

  ~PaymentManager() override;

 private:
  friend class PaymentAppContentUnitTestBase;
  friend class PaymentAppProviderTest;
  friend class PaymentManagerTest;

  // payments::mojom::PaymentManager methods:
  void Init(const GURL& context_url, const std::string& scope) override;
  void DeletePaymentInstrument(
      const std::string& instrument_key,
      DeletePaymentInstrumentCallback callback) override;
  void GetPaymentInstrument(const std::string& instrument_key,
                            GetPaymentInstrumentCallback callback) override;
  void KeysOfPaymentInstruments(
      KeysOfPaymentInstrumentsCallback callback) override;
  void HasPaymentInstrument(const std::string& instrument_key,
                            HasPaymentInstrumentCallback callback) override;
  void SetPaymentInstrument(const std::string& instrument_key,
                            payments::mojom::PaymentInstrumentPtr details,
                            SetPaymentInstrumentCallback callback) override;
  void ClearPaymentInstruments(
      ClearPaymentInstrumentsCallback callback) override;
  void SetUserHint(const std::string& user_hint) override;

  // Called when an error is detected on binding_.
  void OnConnectionError();

  void SetPaymentInstrumentIntermediateCallback(
      PaymentManager::SetPaymentInstrumentCallback callback,
      payments::mojom::PaymentHandlerStatus status);

  // PaymentAppContextImpl owns PaymentManager
  PaymentAppContextImpl* payment_app_context_;

  bool should_set_payment_app_info_;
  GURL context_url_;
  GURL scope_;
  mojo::Binding<payments::mojom::PaymentManager> binding_;
  base::WeakPtrFactory<PaymentManager> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(PaymentManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_PAYMENTS_PAYMENT_MANAGER_H_

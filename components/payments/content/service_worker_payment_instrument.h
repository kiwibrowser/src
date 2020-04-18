// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_SERVICE_WORKER_PAYMENT_INSTRUMENT_H_
#define COMPONENTS_PAYMENTS_CONTENT_SERVICE_WORKER_PAYMENT_INSTRUMENT_H_

#include "components/payments/content/payment_request_spec.h"
#include "components/payments/content/web_app_manifest.h"
#include "components/payments/core/payment_instrument.h"
#include "content/public/browser/stored_payment_app.h"
#include "third_party/blink/public/platform/modules/payments/payment_app.mojom.h"
#include "third_party/blink/public/platform/modules/payments/payment_request.mojom.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace payments {

class PaymentRequestDelegate;

// Represents a service worker based payment app.
class ServiceWorkerPaymentInstrument : public PaymentInstrument {
 public:
  // This constructor is used for a payment app that has been installed in
  // Chrome.
  ServiceWorkerPaymentInstrument(
      content::BrowserContext* browser_context,
      const GURL& top_origin,
      const GURL& frame_origin,
      const PaymentRequestSpec* spec,
      std::unique_ptr<content::StoredPaymentApp> stored_payment_app_info,
      PaymentRequestDelegate* payment_request_delegate);

  // This contructor is used for a payment app that has not been installed in
  // Chrome but can be installed when paying with it.
  ServiceWorkerPaymentInstrument(
      content::WebContents* web_contents,
      const GURL& top_origin,
      const GURL& frame_origin,
      const PaymentRequestSpec* spec,
      std::unique_ptr<WebAppInstallationInfo> installable_payment_app_info,
      const std::string& enabled_methods,
      PaymentRequestDelegate* payment_request_delegate);
  ~ServiceWorkerPaymentInstrument() override;

  // The callback for ValidateCanMakePayment.
  // The first return value is a pointer point to the corresponding
  // ServiceWorkerPaymentInstrument of the result. The second return value is
  // the result.
  using ValidateCanMakePaymentCallback =
      base::OnceCallback<void(ServiceWorkerPaymentInstrument*, bool)>;

  // Validates whether this payment instrument can be used for this payment
  // request. It fires CanMakePaymentEvent to the payment app to do validation.
  // The result is returned through callback.If the returned result is false,
  // then this instrument should not be used for this payment request. This
  // interface must be called before any other interfaces in this class.
  void ValidateCanMakePayment(ValidateCanMakePaymentCallback callback);

  // PaymentInstrument:
  void InvokePaymentApp(Delegate* delegate) override;
  bool IsCompleteForPayment() const override;
  bool IsExactlyMatchingMerchantRequest() const override;
  base::string16 GetMissingInfoLabel() const override;
  bool IsValidForCanMakePayment() const override;
  void RecordUse() override;
  base::string16 GetLabel() const override;
  base::string16 GetSublabel() const override;
  bool IsValidForModifier(const std::vector<std::string>& methods,
                          bool supported_networks_specified,
                          const std::set<std::string>& supported_networks,
                          bool supported_types_specified,
                          const std::set<autofill::CreditCard::CardType>&
                              supported_types) const override;
  const gfx::ImageSkia* icon_image_skia() const override;

 private:
  friend class ServiceWorkerPaymentInstrumentTest;

  void OnPaymentAppInvoked(mojom::PaymentHandlerResponsePtr response);
  mojom::PaymentRequestEventDataPtr CreatePaymentRequestEventData();

  mojom::CanMakePaymentEventDataPtr CreateCanMakePaymentEventData();
  void OnCanMakePayment(ValidateCanMakePaymentCallback callback, bool result);

  content::BrowserContext* browser_context_;
  GURL top_origin_;
  GURL frame_origin_;
  const PaymentRequestSpec* spec_;
  std::unique_ptr<content::StoredPaymentApp> stored_payment_app_info_;
  std::unique_ptr<gfx::ImageSkia> icon_image_;

  // Weak pointer is fine here since the owner of this object is
  // PaymentRequestState which also owns PaymentResponseHelper.
  Delegate* delegate_;

  // Weak pointer that must outlive this object.
  PaymentRequestDelegate* payment_request_delegate_;

  // PaymentAppProvider::CanMakePayment result of this payment instrument.
  bool can_make_payment_result_;

  // Below variables are used for installable ServiceWorkerPaymentInstrument
  // specifically.
  bool needs_installation_;
  content::WebContents* web_contents_;
  std::unique_ptr<WebAppInstallationInfo> installable_web_app_info_;
  std::string installable_enabled_method_;

  base::WeakPtrFactory<ServiceWorkerPaymentInstrument> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerPaymentInstrument);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_SERVICE_WORKER_PAYMENT_INSTRUMENT_H_

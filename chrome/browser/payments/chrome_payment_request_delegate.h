// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAYMENTS_CHROME_PAYMENT_REQUEST_DELEGATE_H_
#define CHROME_BROWSER_PAYMENTS_CHROME_PAYMENT_REQUEST_DELEGATE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/payments/content/content_payment_request_delegate.h"

namespace content {
class WebContents;
}

namespace payments {

class PaymentRequestDialog;

class ChromePaymentRequestDelegate : public ContentPaymentRequestDelegate {
 public:
  explicit ChromePaymentRequestDelegate(content::WebContents* web_contents);
  ~ChromePaymentRequestDelegate() override;

  // PaymentRequestDelegate:
  void ShowDialog(PaymentRequest* request) override;
  void CloseDialog() override;
  void ShowErrorMessage() override;
  void ShowProcessingSpinner() override;
  autofill::PersonalDataManager* GetPersonalDataManager() override;
  const std::string& GetApplicationLocale() const override;
  bool IsIncognito() const override;
  bool IsSslCertificateValid() override;
  const GURL& GetLastCommittedURL() const override;
  void DoFullCardRequest(
      const autofill::CreditCard& credit_card,
      base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
          result_delegate) override;
  autofill::AddressNormalizer* GetAddressNormalizer() override;
  autofill::RegionDataLoader* GetRegionDataLoader() override;
  ukm::UkmRecorder* GetUkmRecorder() override;
  std::string GetAuthenticatedEmail() const override;
  PrefService* GetPrefService() override;
  bool IsBrowserWindowActive() const override;
  scoped_refptr<PaymentManifestWebDataService>
  GetPaymentManifestWebDataService() const override;
  PaymentRequestDisplayManager* GetDisplayManager() override;
  void EmbedPaymentHandlerWindow(
      const GURL& url,
      PaymentHandlerOpenWindowCallback callback) override;

 protected:
  // Reference to the dialog so that we can satisfy calls to CloseDialog(). This
  // reference is invalid once CloseDialog() has been called on it, because the
  // dialog will be destroyed. Owned by the views:: dialog machinery. Protected
  // for testing.
  PaymentRequestDialog* shown_dialog_;

  // The instance of the dialog that was created but not shown yet. Since it
  // hasn't been shown, it's still owned by it's creator. This is non null only
  // when the current Payment Request supports skipping the payment sheet (see
  // PaymentRequest::SatisfiesSkipUIConstraints) and is reset once the
  // underlying pointer becomes owned by the views:: machinery (when the dialog
  // is shown).
  std::unique_ptr<PaymentRequestDialog> hidden_dialog_;

  // Shows |hidden_dialog_| if the current Payment Request doesn't support the
  // skip UI flow. This also transfer its ownership to the views dialog code and
  // keep a reference to it in |shown_dialog_|.
  // Otherwise, this calls Pay() on the current Payment Request to allow the
  // skip UI flow to carry on.
  void MaybeShowHiddenDialog(PaymentRequest* request);

 private:
  // Not owned but outlives the PaymentRequest object that owns this.
  content::WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(ChromePaymentRequestDelegate);
};

}  // namespace payments

#endif  // CHROME_BROWSER_PAYMENTS_CHROME_PAYMENT_REQUEST_DELEGATE_H_

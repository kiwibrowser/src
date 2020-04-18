// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/payments/test_chrome_payment_request_delegate.h"

#include "content/public/browser/web_contents.h"

namespace payments {

TestChromePaymentRequestDelegate::TestChromePaymentRequestDelegate(
    content::WebContents* web_contents,
    PaymentRequestDialogView::ObserverForTest* observer,
    PrefService* pref_service,
    bool is_incognito,
    bool is_valid_ssl,
    bool is_browser_window_active)
    : ChromePaymentRequestDelegate(web_contents),
      region_data_loader_(nullptr),
      observer_(observer),
      pref_service_(pref_service),
      is_incognito_(is_incognito),
      is_valid_ssl_(is_valid_ssl),
      is_browser_window_active_(is_browser_window_active) {}

void TestChromePaymentRequestDelegate::ShowDialog(PaymentRequest* request) {
  hidden_dialog_ =
      std::make_unique<PaymentRequestDialogView>(request, observer_);
  MaybeShowHiddenDialog(request);
}

bool TestChromePaymentRequestDelegate::IsIncognito() const {
  return is_incognito_;
}

bool TestChromePaymentRequestDelegate::IsSslCertificateValid() {
  return is_valid_ssl_;
}

autofill::RegionDataLoader*
TestChromePaymentRequestDelegate::GetRegionDataLoader() {
  if (region_data_loader_)
    return region_data_loader_;
  return ChromePaymentRequestDelegate::GetRegionDataLoader();
}

PrefService* TestChromePaymentRequestDelegate::GetPrefService() {
  return pref_service_;
}

bool TestChromePaymentRequestDelegate::IsBrowserWindowActive() const {
  return is_browser_window_active_;
}

}  // namespace payments

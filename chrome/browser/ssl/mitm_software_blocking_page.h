// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_MITM_SOFTWARE_BLOCKING_PAGE_H_
#define CHROME_BROWSER_SSL_MITM_SOFTWARE_BLOCKING_PAGE_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/ssl/ssl_blocking_page_base.h"
#include "chrome/browser/ssl/ssl_cert_reporter.h"
#include "components/ssl_errors/error_classification.h"
#include "content/public/browser/certificate_request_result_type.h"
#include "net/ssl/ssl_info.h"

class GURL;

namespace security_interstitials {
class MITMSoftwareUI;
}

// This class is responsible for showing/hiding the interstitial page that
// occurs when an SSL error is caused by any sort of MITM software. MITM
// software includes antiviruses, firewalls, proxies or any other non-malicious
// software that intercepts and rewrites the user's connection. This class
// creates the interstitial UI using security_interstitials::MITMSoftwareUI and
// then displays it. It deletes itself when the interstitial page is closed.
class MITMSoftwareBlockingPage : public SSLBlockingPageBase {
 public:
  // Interstitial type, used in tests.
  static const InterstitialPageDelegate::TypeID kTypeForTesting;

  // If the blocking page isn't shown, the caller is responsible for cleaning
  // up the blocking page. Otherwise, the interstitial takes ownership when
  // shown.
  MITMSoftwareBlockingPage(
      content::WebContents* web_contents,
      int cert_error,
      const GURL& request_url,
      std::unique_ptr<SSLCertReporter> ssl_cert_reporter,
      const net::SSLInfo& ssl_info,
      const std::string& mitm_software_name,
      bool is_enterprise_managed,
      const base::Callback<void(content::CertificateRequestResultType)>&
          callback);

  ~MITMSoftwareBlockingPage() override;

  // InterstitialPageDelegate method:
  InterstitialPageDelegate::TypeID GetTypeForTesting() const override;

 protected:
  // InterstitialPageDelegate implementation:
  void CommandReceived(const std::string& command) override;
  void OverrideEntry(content::NavigationEntry* entry) override;
  void OverrideRendererPrefs(content::RendererPreferences* prefs) override;
  void OnDontProceed() override;

  // SecurityInterstitialPage implementation:
  bool ShouldCreateNewNavigation() const override;
  void PopulateInterstitialStrings(
      base::DictionaryValue* load_time_data) override;

 private:
  void NotifyDenyCertificate();

  base::Callback<void(content::CertificateRequestResultType)> callback_;
  const net::SSLInfo ssl_info_;

  const std::unique_ptr<security_interstitials::MITMSoftwareUI>
      mitm_software_ui_;

  DISALLOW_COPY_AND_ASSIGN(MITMSoftwareBlockingPage);
};

#endif  // CHROME_BROWSER_SSL_MITM_SOFTWARE_BLOCKING_PAGE_H_

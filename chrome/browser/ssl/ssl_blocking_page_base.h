// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_SSL_BLOCKING_PAGE_BASE_H_
#define CHROME_BROWSER_SSL_SSL_BLOCKING_PAGE_BASE_H_

#include "chrome/browser/ssl/cert_report_helper.h"
#include "chrome/browser/ssl/certificate_error_report.h"
#include "components/security_interstitials/content/security_interstitial_page.h"

namespace base {
class Time;
}  // namespace base

namespace net {
class SSLInfo;
}  // namespace net

class CertReportHelper;
class SSLCertReporter;

// This is the base class for blocking pages representing SSL certificate
// errors.
class SSLBlockingPageBase
    : public security_interstitials::SecurityInterstitialPage {
 public:
  SSLBlockingPageBase(
      content::WebContents* web_contents,
      int cert_error,
      CertificateErrorReport::InterstitialReason interstitial_reason,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      std::unique_ptr<SSLCertReporter> ssl_cert_reporter,
      bool overridable,
      const base::Time& time_triggered,
      std::unique_ptr<
          security_interstitials::SecurityInterstitialControllerClient>
          controller_client);
  ~SSLBlockingPageBase() override;

  // security_interstitials::SecurityInterstitialPage:
  void OnInterstitialClosing() override;

  void SetSSLCertReporterForTesting(
      std::unique_ptr<SSLCertReporter> ssl_cert_reporter);

 protected:
  CertReportHelper* cert_report_helper();

 private:
  const std::unique_ptr<CertReportHelper> cert_report_helper_;
  DISALLOW_COPY_AND_ASSIGN(SSLBlockingPageBase);
};

#endif  // CHROME_BROWSER_SSL_SSL_BLOCKING_PAGE_BASE_H_

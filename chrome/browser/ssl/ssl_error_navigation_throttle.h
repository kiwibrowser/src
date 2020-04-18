// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_SSL_ERROR_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_SSL_SSL_ERROR_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ssl/ssl_error_handler.h"
#include "content/public/browser/navigation_throttle.h"

class SSLCertReporter;

namespace content {
class NavigationHandle;
}  // namespace content

namespace security_interstitials {
class SecurityInterstitialPage;
}  // namespace security_interstitials

// SSLErrorNavigationThrottle watches for failed navigations that should be
// displayed as SSL interstitial pages. More specifically,
// SSLErrorNavigationThrottle::WillFailRequest() will defer any navigations that
// failed due to a certificate error. After calculating which interstitial to
// show, it will cancel the navigation with the interstitial's custom error page
// HTML.
class SSLErrorNavigationThrottle : public content::NavigationThrottle {
 public:
  typedef base::OnceCallback<void(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      bool expired_previous_decision,
      std::unique_ptr<SSLCertReporter> ssl_cert_reporter,
      const base::Callback<void(content::CertificateRequestResultType)>&
          decision_callback,
      base::OnceCallback<void(
          std::unique_ptr<security_interstitials::SecurityInterstitialPage>)>
          blocking_page_ready_callback)>
      HandleSSLErrorCallback;

  explicit SSLErrorNavigationThrottle(
      content::NavigationHandle* handle,
      std::unique_ptr<SSLCertReporter> ssl_cert_reporter,
      HandleSSLErrorCallback handle_ssl_error_callback);
  ~SSLErrorNavigationThrottle() override;

  // content::NavigationThrottle:
  ThrottleCheckResult WillFailRequest() override;
  ThrottleCheckResult WillProcessResponse() override;
  const char* GetNameForLogging() override;

 private:
  void QueueShowInterstitial(
      HandleSSLErrorCallback handle_ssl_error_callback,
      content::WebContents* web_contents,
      int cert_status,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      std::unique_ptr<SSLCertReporter> ssl_cert_reporter);
  void ShowInterstitial(
      std::unique_ptr<security_interstitials::SecurityInterstitialPage>
          blocking_page);

  std::unique_ptr<SSLCertReporter> ssl_cert_reporter_;
  HandleSSLErrorCallback handle_ssl_error_callback_;
  base::WeakPtrFactory<SSLErrorNavigationThrottle> weak_ptr_factory_;
};

#endif  // CHROME_BROWSER_SSL_SSL_ERROR_NAVIGATION_THROTTLE_H_

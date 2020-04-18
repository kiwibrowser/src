// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_error_navigation_throttle.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/common/chrome_switches.h"
#include "components/security_interstitials/content/security_interstitial_page.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "content/public/browser/navigation_handle.h"
#include "net/cert/cert_status_flags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/extensions/hosted_app_browser_controller.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

SSLErrorNavigationThrottle::SSLErrorNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    std::unique_ptr<SSLCertReporter> ssl_cert_reporter,
    SSLErrorNavigationThrottle::HandleSSLErrorCallback
        handle_ssl_error_callback)
    : content::NavigationThrottle(navigation_handle),
      ssl_cert_reporter_(std::move(ssl_cert_reporter)),
      handle_ssl_error_callback_(std::move(handle_ssl_error_callback)),
      weak_ptr_factory_(this) {}

SSLErrorNavigationThrottle::~SSLErrorNavigationThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
SSLErrorNavigationThrottle::WillFailRequest() {
  DCHECK(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kCommittedInterstitials));
  content::NavigationHandle* handle = navigation_handle();
  // If there was no certificate error, SSLInfo will be empty.
  int cert_status = handle->GetSSLInfo().cert_status;
  if (!net::IsCertStatusError(cert_status) ||
      net::IsCertStatusMinorError(cert_status)) {
    return content::NavigationThrottle::PROCEED;
  }

  // Do not set special error page HTML for subframes; those are handled as
  // normal network errors.
  if (!handle->IsInMainFrame()) {
    return content::NavigationThrottle::PROCEED;
  }

  QueueShowInterstitial(std::move(handle_ssl_error_callback_),
                        handle->GetWebContents(), cert_status,
                        handle->GetSSLInfo(), handle->GetURL(),
                        std::move(ssl_cert_reporter_));
  return content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::DEFER);
}

content::NavigationThrottle::ThrottleCheckResult
SSLErrorNavigationThrottle::WillProcessResponse() {
  DCHECK(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kCommittedInterstitials));
  content::NavigationHandle* handle = navigation_handle();
  // If there was no certificate error, SSLInfo will be empty.
  int cert_status = handle->GetSSLInfo().cert_status;
  if (!net::IsCertStatusError(cert_status) ||
      net::IsCertStatusMinorError(cert_status)) {
    return content::NavigationThrottle::PROCEED;
  }

  // Do not set special error page HTML for subframes; those are handled as
  // normal network errors.
  if (!handle->IsInMainFrame()) {
    return content::NavigationThrottle::PROCEED;
  }

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Hosted Apps should not be allowed to run if there is a problem with their
  // certificate. So, when a user tries to open such an app, we show an
  // interstitial, even if the user has previously clicked through one. Clicking
  // through the interstitial will continue the navigation in a regular browser
  // window.
  Browser* browser =
      chrome::FindBrowserWithWebContents(handle->GetWebContents());
  if (browser &&
      extensions::HostedAppBrowserController::IsForExperimentalHostedAppBrowser(
          browser)) {
    QueueShowInterstitial(std::move(handle_ssl_error_callback_),
                          handle->GetWebContents(), cert_status,
                          handle->GetSSLInfo(), handle->GetURL(),
                          std::move(ssl_cert_reporter_));
    return content::NavigationThrottle::ThrottleCheckResult(
        content::NavigationThrottle::DEFER);
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

  return content::NavigationThrottle::PROCEED;
}

const char* SSLErrorNavigationThrottle::GetNameForLogging() {
  return "SSLErrorNavigationThrottle";
}

void SSLErrorNavigationThrottle::QueueShowInterstitial(
    HandleSSLErrorCallback handle_ssl_error_callback,
    content::WebContents* web_contents,
    int cert_status,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    std::unique_ptr<SSLCertReporter> ssl_cert_reporter) {
  // We don't know whether SSLErrorHandler will call the ShowInterstitial()
  // callback synchronously, so we post a task that will run after the caller
  // defers the navigation. This ensures that ShowInterstitial() can always
  // safely call CancelDeferredNavigation().
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(
          std::move(handle_ssl_error_callback), web_contents,
          net::MapCertStatusToNetError(cert_status), ssl_info, request_url,
          false /* expired_previous_decision */, std::move(ssl_cert_reporter),
          base::Callback<void(content::CertificateRequestResultType)>(),
          base::BindOnce(&SSLErrorNavigationThrottle::ShowInterstitial,
                         weak_ptr_factory_.GetWeakPtr())));
}

void SSLErrorNavigationThrottle::ShowInterstitial(
    std::unique_ptr<security_interstitials::SecurityInterstitialPage>
        blocking_page) {
  content::NavigationHandle* handle = navigation_handle();
  int net_error =
      net::MapCertStatusToNetError(handle->GetSSLInfo().cert_status);

  // Get the error page content before giving up ownership of |blocking_page|.
  std::string error_page_content = blocking_page->GetHTMLContents();

  security_interstitials::SecurityInterstitialTabHelper::AssociateBlockingPage(
      handle->GetWebContents(), handle->GetNavigationId(),
      std::move(blocking_page));

  CancelDeferredNavigation(content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::CANCEL, static_cast<net::Error>(net_error),
      error_page_content));
}

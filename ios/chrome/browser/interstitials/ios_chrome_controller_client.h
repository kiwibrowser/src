// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_INTERSTITIALS_IOS_CHROME_CONTROLLER_CLIENT_H_
#define IOS_CHROME_BROWSER_INTERSTITIALS_IOS_CHROME_CONTROLLER_CLIENT_H_

#include <string>

#include "base/macros.h"
#include "components/security_interstitials/core/controller_client.h"

class GURL;

namespace security_interstitials {
class MetricsHelper;
}

namespace web {
class WebInterstitial;
class WebState;
}

// Provides embedder-specific logic for the security error page controller.
class IOSChromeControllerClient
    : public security_interstitials::ControllerClient {
 public:
  IOSChromeControllerClient(
      web::WebState* web_state,
      std::unique_ptr<security_interstitials::MetricsHelper> metrics_helper);
  ~IOSChromeControllerClient() override;

  void SetWebInterstitial(web::WebInterstitial* web_interstitial);

 private:
  // security_interstitials::ControllerClient implementation.
  bool CanLaunchDateAndTimeSettings() override;
  void LaunchDateAndTimeSettings() override;
  void GoBack() override;
  bool CanGoBack() override;
  void GoBackAfterNavigationCommitted() override;
  void Proceed() override;
  void Reload() override;
  void OpenUrlInCurrentTab(const GURL& url) override;
  void OpenUrlInNewForegroundTab(const GURL& url) override;
  const std::string& GetApplicationLocale() const override;
  PrefService* GetPrefService() override;
  const std::string GetExtendedReportingPrefName() const override;

  web::WebState* web_state_;
  web::WebInterstitial* web_interstitial_;

  DISALLOW_COPY_AND_ASSIGN(IOSChromeControllerClient);
};

#endif  // IOS_CHROME_BROWSER_INTERSTITIALS_IOS_CHROME_CONTROLLER_CLIENT_H_

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_IOS_CHROME_SIGNIN_STATUS_METRICS_PROVIDER_DELEGATE_H_
#define IOS_CHROME_BROWSER_SIGNIN_IOS_CHROME_SIGNIN_STATUS_METRICS_PROVIDER_DELEGATE_H_

#include <vector>

#include "base/macros.h"
#include "components/signin/core/browser/signin_status_metrics_provider_delegate.h"
#include "ios/chrome/browser/signin/signin_manager_factory_observer.h"

namespace ios {
class ChromeBrowserState;
}

class IOSChromeSigninStatusMetricsProviderDelegate
    : public SigninStatusMetricsProviderDelegate,
      public SigninManagerFactoryObserver {
 public:
  IOSChromeSigninStatusMetricsProviderDelegate();
  ~IOSChromeSigninStatusMetricsProviderDelegate() override;

 private:
  // SigninStatusMetricsProviderDelegate implementation.
  void Initialize() override;
  AccountsStatus GetStatusOfAllAccounts() override;
  std::vector<SigninManager*> GetSigninManagersForAllAccounts() override;

  // SigninManagerFactoryObserver implementation.
  void SigninManagerCreated(SigninManager* manager) override;
  void SigninManagerShutdown(SigninManager* manager) override;

  // Returns the loaded ChromeBrowserState instances.
  std::vector<ios::ChromeBrowserState*> GetLoadedChromeBrowserStates();

  DISALLOW_COPY_AND_ASSIGN(IOSChromeSigninStatusMetricsProviderDelegate);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_IOS_CHROME_SIGNIN_STATUS_METRICS_PROVIDER_DELEGATE_H_

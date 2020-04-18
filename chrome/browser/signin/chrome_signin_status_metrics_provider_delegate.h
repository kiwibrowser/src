// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_CHROME_SIGNIN_STATUS_METRICS_PROVIDER_DELEGATE_H_
#define CHROME_BROWSER_SIGNIN_CHROME_SIGNIN_STATUS_METRICS_PROVIDER_DELEGATE_H_

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "components/signin/core/browser/signin_status_metrics_provider_delegate.h"

class ChromeSigninStatusMetricsProviderDelegate
    : public SigninStatusMetricsProviderDelegate,
      public BrowserListObserver,
      public SigninManagerFactory::Observer {
 public:
  ChromeSigninStatusMetricsProviderDelegate();
  ~ChromeSigninStatusMetricsProviderDelegate() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(ChromeSigninStatusMetricsProviderDelegateTest,
                           UpdateStatusWhenBrowserAdded);

  // SigninStatusMetricsProviderDelegate:
  void Initialize() override;
  AccountsStatus GetStatusOfAllAccounts() override;
  std::vector<SigninManager*> GetSigninManagersForAllAccounts() override;

  // BrowserListObserver:
  void OnBrowserAdded(Browser* browser) override;

  // SigninManagerFactoryObserver:
  void SigninManagerCreated(SigninManagerBase* manager) override;
  void SigninManagerShutdown(SigninManagerBase* manager) override;

  // Updates the sign-in status right after a new browser is opened.
  void UpdateStatusWhenBrowserAdded(bool signed_in);

  DISALLOW_COPY_AND_ASSIGN(ChromeSigninStatusMetricsProviderDelegate);
};

#endif  // CHROME_BROWSER_SIGNIN_CHROME_SIGNIN_STATUS_METRICS_PROVIDER_DELEGATE_H_

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/ios_chrome_signin_status_metrics_provider_delegate.h"

#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/browser/signin_status_metrics_provider.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"

IOSChromeSigninStatusMetricsProviderDelegate::
    IOSChromeSigninStatusMetricsProviderDelegate() {}

IOSChromeSigninStatusMetricsProviderDelegate::
    ~IOSChromeSigninStatusMetricsProviderDelegate() {
  ios::SigninManagerFactory* factory = ios::SigninManagerFactory::GetInstance();
  if (factory)
    factory->RemoveObserver(this);
}

void IOSChromeSigninStatusMetricsProviderDelegate::Initialize() {
  ios::SigninManagerFactory* factory = ios::SigninManagerFactory::GetInstance();
  if (factory)
    factory->AddObserver(this);
}

AccountsStatus
IOSChromeSigninStatusMetricsProviderDelegate::GetStatusOfAllAccounts() {
  std::vector<ios::ChromeBrowserState*> browser_state_list =
      GetLoadedChromeBrowserStates();
  AccountsStatus accounts_status;
  accounts_status.num_accounts = browser_state_list.size();
  accounts_status.num_opened_accounts = accounts_status.num_accounts;

  for (ios::ChromeBrowserState* browser_state : browser_state_list) {
    SigninManager* manager = ios::SigninManagerFactory::GetForBrowserState(
        browser_state->GetOriginalChromeBrowserState());
    if (manager && manager->IsAuthenticated())
      accounts_status.num_signed_in_accounts++;
  }

  return accounts_status;
}

std::vector<SigninManager*> IOSChromeSigninStatusMetricsProviderDelegate::
    GetSigninManagersForAllAccounts() {
  std::vector<SigninManager*> managers;
  for (ios::ChromeBrowserState* browser_state :
       GetLoadedChromeBrowserStates()) {
    SigninManager* manager =
        ios::SigninManagerFactory::GetForBrowserStateIfExists(browser_state);
    if (manager) {
      managers.push_back(manager);
    }
  }

  return managers;
}

void IOSChromeSigninStatusMetricsProviderDelegate::SigninManagerCreated(
    SigninManager* manager) {
  owner()->OnSigninManagerCreated(manager);
}

void IOSChromeSigninStatusMetricsProviderDelegate::SigninManagerShutdown(
    SigninManager* manager) {
  owner()->OnSigninManagerShutdown(manager);
}

std::vector<ios::ChromeBrowserState*>
IOSChromeSigninStatusMetricsProviderDelegate::GetLoadedChromeBrowserStates() {
  return GetApplicationContext()
      ->GetChromeBrowserStateManager()
      ->GetLoadedBrowserStates();
}

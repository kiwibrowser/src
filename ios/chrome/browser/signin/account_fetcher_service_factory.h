// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_ACCOUNT_FETCHER_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_SIGNIN_ACCOUNT_FETCHER_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class AccountFetcherService;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace ios {

class ChromeBrowserState;

class AccountFetcherServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static AccountFetcherService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static AccountFetcherServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<AccountFetcherServiceFactory>;

  AccountFetcherServiceFactory();
  ~AccountFetcherServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(AccountFetcherServiceFactory);
};

}  // namespace ios

#endif  // IOS_CHROME_BROWSER_SIGNIN_ACCOUNT_FETCHER_SERVICE_FACTORY_H_

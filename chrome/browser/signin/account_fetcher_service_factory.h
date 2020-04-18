// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_ACCOUNT_FETCHER_SERVICE_FACTORY_H_
#define CHROME_BROWSER_SIGNIN_ACCOUNT_FETCHER_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class AccountFetcherService;
class Profile;

class AccountFetcherServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static AccountFetcherService* GetForProfile(Profile* profile);
  static AccountFetcherServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<AccountFetcherServiceFactory>;

  AccountFetcherServiceFactory();
  ~AccountFetcherServiceFactory() override;

  // BrowserContextKeyedServiceFactory implementation
  void RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(AccountFetcherServiceFactory);
};

#endif  // CHROME_BROWSER_SIGNIN_ACCOUNT_FETCHER_SERVICE_FACTORY_H_

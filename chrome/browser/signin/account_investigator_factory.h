// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIGNIN_ACCOUNT_INVESTIGATOR_FACTORY_H_
#define CHROME_BROWSER_SIGNIN_ACCOUNT_INVESTIGATOR_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;
class AccountInvestigator;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

// Factory for BrowserKeyedService AccountInvestigator.
class AccountInvestigatorFactory : public BrowserContextKeyedServiceFactory {
 public:
  static AccountInvestigator* GetForProfile(Profile* profile);

  static AccountInvestigatorFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<AccountInvestigatorFactory>;

  AccountInvestigatorFactory();
  ~AccountInvestigatorFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(AccountInvestigatorFactory);
};

#endif  // CHROME_BROWSER_SIGNIN_ACCOUNT_INVESTIGATOR_FACTORY_H_

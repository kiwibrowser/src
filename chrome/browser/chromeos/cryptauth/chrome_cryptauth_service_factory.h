// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CRYPTAUTH_CHROME_CRYPTAUTH_SERVICE_FACTORY_H_
#define CHROME_BROWSER_CHROMEOS_CRYPTAUTH_CHROME_CRYPTAUTH_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/cryptauth/cryptauth_service.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace chromeos {

// Factory which is used to access the CryptAuthService singleton.
class ChromeCryptAuthServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static cryptauth::CryptAuthService* GetForBrowserContext(
      content::BrowserContext* context);

  static ChromeCryptAuthServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<ChromeCryptAuthServiceFactory>;

  ChromeCryptAuthServiceFactory();
  ~ChromeCryptAuthServiceFactory() override;

  // BrowserContextKeyedServiceFactory
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  DISALLOW_COPY_AND_ASSIGN(ChromeCryptAuthServiceFactory);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CRYPTAUTH_CHROME_CRYPTAUTH_SERVICE_FACTORY_H_

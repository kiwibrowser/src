// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PLATFORM_KEYS_PLATFORM_KEYS_SERVICE_FACTORY_H_
#define CHROME_BROWSER_CHROMEOS_PLATFORM_KEYS_PLATFORM_KEYS_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace chromeos {

class PlatformKeysService;

// Factory to create PlatformKeysService.
class PlatformKeysServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static PlatformKeysService* GetForBrowserContext(
      content::BrowserContext* context);

  static PlatformKeysServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<PlatformKeysServiceFactory>;

  PlatformKeysServiceFactory();
  ~PlatformKeysServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(PlatformKeysServiceFactory);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PLATFORM_KEYS_PLATFORM_KEYS_SERVICE_FACTORY_H_

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_REGISTRY_SERVICE_FACTORY_H_
#define CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_REGISTRY_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace crostini {

class CrostiniRegistryService;

class CrostiniRegistryServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static CrostiniRegistryService* GetForProfile(Profile* profile);
  static CrostiniRegistryServiceFactory* GetInstance();

 private:
  friend class base::NoDestructor<CrostiniRegistryServiceFactory>;

  CrostiniRegistryServiceFactory();
  ~CrostiniRegistryServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(CrostiniRegistryServiceFactory);
};

}  // namespace crostini

#endif  // CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_REGISTRY_SERVICE_FACTORY_H_

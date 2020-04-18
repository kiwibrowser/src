// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CONTENT_BROWSER_PASSWORD_MANAGER_INTERNALS_SERVICE_FACTORY_H_
#define COMPONENTS_PASSWORD_MANAGER_CONTENT_BROWSER_PASSWORD_MANAGER_INTERNALS_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}

namespace password_manager {

class PasswordManagerInternalsService;

// BrowserContextKeyedServiceFactory for PasswordManagerInternalsService. It
// does not override BrowserContextKeyedServiceFactory::GetBrowserContextToUse,
// which means that no service is returned in Incognito.
class PasswordManagerInternalsServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static PasswordManagerInternalsService* GetForBrowserContext(
      content::BrowserContext* context);

  static PasswordManagerInternalsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      PasswordManagerInternalsServiceFactory>;

  PasswordManagerInternalsServiceFactory();
  ~PasswordManagerInternalsServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerInternalsServiceFactory);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CONTENT_BROWSER_PASSWORD_MANAGER_INTERNALS_SERVICE_FACTORY_H_

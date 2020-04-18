// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/content/browser/password_manager_internals_service_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/password_manager/core/browser/password_manager_internals_service.h"

namespace password_manager {

// static
PasswordManagerInternalsService*
PasswordManagerInternalsServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<PasswordManagerInternalsService*>(
      GetInstance()->GetServiceForBrowserContext(context, /* create = */ true));
}

// static
PasswordManagerInternalsServiceFactory*
PasswordManagerInternalsServiceFactory::GetInstance() {
  return base::Singleton<PasswordManagerInternalsServiceFactory>::get();
}

PasswordManagerInternalsServiceFactory::PasswordManagerInternalsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "PasswordManagerInternalsService",
          BrowserContextDependencyManager::GetInstance()) {
}

PasswordManagerInternalsServiceFactory::
    ~PasswordManagerInternalsServiceFactory() {
}

KeyedService* PasswordManagerInternalsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* /* context */) const {
  return new PasswordManagerInternalsService();
}

}  // namespace password_manager

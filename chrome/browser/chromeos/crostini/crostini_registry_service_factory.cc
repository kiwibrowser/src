// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/crostini/crostini_registry_service_factory.h"

#include "chrome/browser/chromeos/crostini/crostini_registry_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace crostini {

// static
CrostiniRegistryService* CrostiniRegistryServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<CrostiniRegistryService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
CrostiniRegistryServiceFactory* CrostiniRegistryServiceFactory::GetInstance() {
  static base::NoDestructor<CrostiniRegistryServiceFactory> factory;
  return factory.get();
}

CrostiniRegistryServiceFactory::CrostiniRegistryServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "CrostiniRegistryService",
          BrowserContextDependencyManager::GetInstance()) {}

CrostiniRegistryServiceFactory::~CrostiniRegistryServiceFactory() = default;

KeyedService* CrostiniRegistryServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  return new CrostiniRegistryService(profile);
}

}  // namespace crostini

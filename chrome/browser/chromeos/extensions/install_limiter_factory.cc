// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/install_limiter_factory.h"

#include "chrome/browser/chromeos/extensions/install_limiter.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"

namespace extensions {

// static
InstallLimiter* InstallLimiterFactory::GetForProfile(Profile* profile) {
  return static_cast<InstallLimiter*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
InstallLimiterFactory* InstallLimiterFactory::GetInstance() {
  return base::Singleton<InstallLimiterFactory>::get();
}

InstallLimiterFactory::InstallLimiterFactory()
    : BrowserContextKeyedServiceFactory(
        "InstallLimiter",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
}

InstallLimiterFactory::~InstallLimiterFactory() {
}

KeyedService* InstallLimiterFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new InstallLimiter();
}

}  // namespace extensions

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/extensions/cast_extension_system_factory.h"

#include "chromecast/browser/extensions/cast_extension_system.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/browser/extension_registry_factory.h"

using content::BrowserContext;

namespace extensions {

ExtensionSystem* CastExtensionSystemFactory::GetForBrowserContext(
    BrowserContext* context) {
  return static_cast<CastExtensionSystem*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
CastExtensionSystemFactory* CastExtensionSystemFactory::GetInstance() {
  return base::Singleton<CastExtensionSystemFactory>::get();
}

CastExtensionSystemFactory::CastExtensionSystemFactory()
    : ExtensionSystemProvider("CastExtensionSystem",
                              BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ExtensionPrefsFactory::GetInstance());
  DependsOn(ExtensionRegistryFactory::GetInstance());
}

CastExtensionSystemFactory::~CastExtensionSystemFactory() {}

KeyedService* CastExtensionSystemFactory::BuildServiceInstanceFor(
    BrowserContext* context) const {
  return new CastExtensionSystem(context);
}

BrowserContext* CastExtensionSystemFactory::GetBrowserContextToUse(
    BrowserContext* context) const {
  // Use a separate instance for incognito.
  return context;
}

bool CastExtensionSystemFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace extensions

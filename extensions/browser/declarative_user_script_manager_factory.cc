// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/declarative_user_script_manager_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/declarative_user_script_manager.h"
#include "extensions/browser/extension_registry_factory.h"
#include "extensions/browser/extensions_browser_client.h"

using content::BrowserContext;

namespace extensions {

// static
DeclarativeUserScriptManager*
DeclarativeUserScriptManagerFactory::GetForBrowserContext(
    BrowserContext* context) {
  return static_cast<DeclarativeUserScriptManager*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
DeclarativeUserScriptManagerFactory*
DeclarativeUserScriptManagerFactory::GetInstance() {
  return base::Singleton<DeclarativeUserScriptManagerFactory>::get();
}

DeclarativeUserScriptManagerFactory::DeclarativeUserScriptManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "DeclarativeUserScriptManager",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ExtensionRegistryFactory::GetInstance());
}

DeclarativeUserScriptManagerFactory::~DeclarativeUserScriptManagerFactory() {
}

KeyedService* DeclarativeUserScriptManagerFactory::BuildServiceInstanceFor(
    BrowserContext* context) const {
  return new DeclarativeUserScriptManager(context);
}

BrowserContext* DeclarativeUserScriptManagerFactory::GetBrowserContextToUse(
    BrowserContext* context) const {
  // Redirected in incognito.
  return ExtensionsBrowserClient::Get()->GetOriginalContext(context);
}

}  // namespace extensions

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/error_console/error_console_factory.h"

#include "chrome/browser/extensions/error_console/error_console.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/extension_registry_factory.h"
#include "extensions/browser/extensions_browser_client.h"

using content::BrowserContext;

namespace extensions {

// static
ErrorConsole* ErrorConsoleFactory::GetForBrowserContext(
    BrowserContext* context) {
  return static_cast<ErrorConsole*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
ErrorConsoleFactory* ErrorConsoleFactory::GetInstance() {
  return base::Singleton<ErrorConsoleFactory>::get();
}

ErrorConsoleFactory::ErrorConsoleFactory()
    : BrowserContextKeyedServiceFactory(
          "ErrorConsole",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ExtensionRegistryFactory::GetInstance());
}

ErrorConsoleFactory::~ErrorConsoleFactory() {
}

KeyedService* ErrorConsoleFactory::BuildServiceInstanceFor(
    BrowserContext* context) const {
  return new ErrorConsole(Profile::FromBrowserContext(context));
}

BrowserContext* ErrorConsoleFactory::GetBrowserContextToUse(
    BrowserContext* context) const {
  // Redirected in incognito.
  return ExtensionsBrowserClient::Get()->GetOriginalContext(context);
}

}  // namespace extensions

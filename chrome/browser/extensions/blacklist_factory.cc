// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/blacklist.h"
#include "chrome/browser/extensions/blacklist_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/browser/extensions_browser_client.h"

using content::BrowserContext;

namespace extensions {

// static
Blacklist* BlacklistFactory::GetForBrowserContext(BrowserContext* context) {
  return static_cast<Blacklist*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
BlacklistFactory* BlacklistFactory::GetInstance() {
  return base::Singleton<BlacklistFactory>::get();
}

BlacklistFactory::BlacklistFactory()
    : BrowserContextKeyedServiceFactory(
          "Blacklist",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(extensions::ExtensionPrefsFactory::GetInstance());
}

BlacklistFactory::~BlacklistFactory() {
}

KeyedService* BlacklistFactory::BuildServiceInstanceFor(
    BrowserContext* context) const {
  return new Blacklist(ExtensionPrefs::Get(context));
}

BrowserContext* BlacklistFactory::GetBrowserContextToUse(
    BrowserContext* context) const {
  // Redirected in incognito.
  return ExtensionsBrowserClient::Get()->GetOriginalContext(context);
}

}  // namespace extensions


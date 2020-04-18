// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/display_source/display_source_event_router_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/api/display_source/display_source_connection_delegate_factory.h"
#include "extensions/browser/api/display_source/display_source_event_router.h"
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"

namespace extensions {

// static
DisplaySourceEventRouter* DisplaySourceEventRouterFactory::GetForProfile(
    content::BrowserContext* context) {
  return static_cast<DisplaySourceEventRouter*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
DisplaySourceEventRouterFactory*
DisplaySourceEventRouterFactory::GetInstance() {
  return base::Singleton<DisplaySourceEventRouterFactory>::get();
}

DisplaySourceEventRouterFactory::DisplaySourceEventRouterFactory()
    : BrowserContextKeyedServiceFactory(
          "DisplaySourceEventRouter",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
  DependsOn(DisplaySourceConnectionDelegateFactory::GetInstance());
}

DisplaySourceEventRouterFactory::~DisplaySourceEventRouterFactory() {}

KeyedService* DisplaySourceEventRouterFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return DisplaySourceEventRouter::Create(context);
}

content::BrowserContext*
DisplaySourceEventRouterFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return ExtensionsBrowserClient::Get()->GetOriginalContext(context);
}

bool DisplaySourceEventRouterFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

bool DisplaySourceEventRouterFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace extensions

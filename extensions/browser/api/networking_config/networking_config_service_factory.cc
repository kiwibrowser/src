// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/networking_config/networking_config_service_factory.h"

#include <string>

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/api/networking_config/networking_config_service.h"
#include "extensions/browser/extension_registry_factory.h"
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/common/api/networking_config.h"

namespace extensions {

namespace {

class DefaultEventDelegate : public NetworkingConfigService::EventDelegate {
 public:
  explicit DefaultEventDelegate(content::BrowserContext* context);
  ~DefaultEventDelegate() override;

  bool HasExtensionRegisteredForEvent(
      const std::string& extension_id) const override;

 private:
  content::BrowserContext* const context_;
};

DefaultEventDelegate::DefaultEventDelegate(content::BrowserContext* context)
    : context_(context) {
}

DefaultEventDelegate::~DefaultEventDelegate() {
}

bool DefaultEventDelegate::HasExtensionRegisteredForEvent(
    const std::string& extension_id) const {
  return EventRouter::Get(context_)->ExtensionHasEventListener(
      extension_id,
      api::networking_config::OnCaptivePortalDetected::kEventName);
}

}  // namespace

// static
NetworkingConfigService* NetworkingConfigServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<NetworkingConfigService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
NetworkingConfigServiceFactory* NetworkingConfigServiceFactory::GetInstance() {
  return base::Singleton<NetworkingConfigServiceFactory>::get();
}

NetworkingConfigServiceFactory::NetworkingConfigServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "NetworkingConfigService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
  DependsOn(extensions::ExtensionRegistryFactory::GetInstance());
}

NetworkingConfigServiceFactory::~NetworkingConfigServiceFactory() {
}

KeyedService* NetworkingConfigServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new NetworkingConfigService(
      context, std::make_unique<DefaultEventDelegate>(context),
      ExtensionRegistry::Get(context));
}

content::BrowserContext* NetworkingConfigServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return ExtensionsBrowserClient::Get()->GetOriginalContext(context);
}

}  // namespace extensions

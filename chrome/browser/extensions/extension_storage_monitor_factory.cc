// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_storage_monitor_factory.h"

#include "chrome/browser/extensions/extension_storage_monitor.h"
#include "chrome/browser/extensions/extension_system_factory.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/browser/extensions_browser_client.h"

namespace extensions {

// static
ExtensionStorageMonitor*
ExtensionStorageMonitorFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ExtensionStorageMonitor*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
ExtensionStorageMonitorFactory* ExtensionStorageMonitorFactory::GetInstance() {
  return base::Singleton<ExtensionStorageMonitorFactory>::get();
}

ExtensionStorageMonitorFactory::ExtensionStorageMonitorFactory()
    : BrowserContextKeyedServiceFactory(
          "ExtensionStorageMonitor",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
  DependsOn(ExtensionPrefsFactory::GetInstance());
  DependsOn(NotificationDisplayServiceFactory::GetInstance());
}

ExtensionStorageMonitorFactory::~ExtensionStorageMonitorFactory() {
}

KeyedService* ExtensionStorageMonitorFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new ExtensionStorageMonitor(Profile::FromBrowserContext(context));
}

content::BrowserContext* ExtensionStorageMonitorFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return ExtensionsBrowserClient::Get()->GetOriginalContext(context);
}

bool ExtensionStorageMonitorFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

bool ExtensionStorageMonitorFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace extensions

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/content/factory/navigation_monitor_factory.h"

#include "components/download/internal/background_service/navigation_monitor_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace download {

// static
NavigationMonitorFactory* NavigationMonitorFactory::GetInstance() {
  return base::Singleton<NavigationMonitorFactory>::get();
}

download::NavigationMonitor* NavigationMonitorFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<download::NavigationMonitor*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

NavigationMonitorFactory::NavigationMonitorFactory()
    : BrowserContextKeyedServiceFactory(
          "download::NavigationMonitor",
          BrowserContextDependencyManager::GetInstance()) {}

NavigationMonitorFactory::~NavigationMonitorFactory() = default;

KeyedService* NavigationMonitorFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new NavigationMonitorImpl();
}

content::BrowserContext* NavigationMonitorFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}

}  // namespace download

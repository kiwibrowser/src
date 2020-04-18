// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/in_process_service_factory_factory.h"

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "services/preferences/public/cpp/in_process_service_factory.h"
#include "services/service_manager/public/cpp/service.h"

// static
InProcessPrefServiceFactoryFactory*
InProcessPrefServiceFactoryFactory::GetInstance() {
  return base::Singleton<InProcessPrefServiceFactoryFactory>::get();
}

// static
prefs::InProcessPrefServiceFactory*
InProcessPrefServiceFactoryFactory::GetInstanceForContext(
    content::BrowserContext* context) {
  return static_cast<prefs::InProcessPrefServiceFactory*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

InProcessPrefServiceFactoryFactory::InProcessPrefServiceFactoryFactory()
    : BrowserContextKeyedServiceFactory(
          "InProcessPrefServiceFactory",
          BrowserContextDependencyManager::GetInstance()) {}

InProcessPrefServiceFactoryFactory::~InProcessPrefServiceFactoryFactory() =
    default;

KeyedService* InProcessPrefServiceFactoryFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new prefs::InProcessPrefServiceFactory;
}

content::BrowserContext*
InProcessPrefServiceFactoryFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}

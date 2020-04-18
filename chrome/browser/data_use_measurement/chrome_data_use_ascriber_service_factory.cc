// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/data_use_measurement/chrome_data_use_ascriber_service_factory.h"

#include "base/bind.h"
#include "chrome/browser/data_use_measurement/chrome_data_use_ascriber_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace data_use_measurement {

// static
ChromeDataUseAscriberService*
ChromeDataUseAscriberServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ChromeDataUseAscriberService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
ChromeDataUseAscriberServiceFactory*
ChromeDataUseAscriberServiceFactory::GetInstance() {
  return base::Singleton<ChromeDataUseAscriberServiceFactory>::get();
}

ChromeDataUseAscriberServiceFactory::ChromeDataUseAscriberServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ChromeDataUseAscriberService",
          BrowserContextDependencyManager::GetInstance()) {}

ChromeDataUseAscriberServiceFactory::~ChromeDataUseAscriberServiceFactory() {}

KeyedService* ChromeDataUseAscriberServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new ChromeDataUseAscriberService();
}

}  // namespace data_use_measurement

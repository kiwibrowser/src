// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_STORAGE_MONITOR_FACTORY_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_STORAGE_MONITOR_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace extensions {

class ExtensionStorageMonitor;

class ExtensionStorageMonitorFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static ExtensionStorageMonitor* GetForBrowserContext(
      content::BrowserContext* context);

  static ExtensionStorageMonitorFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<ExtensionStorageMonitorFactory>;

  ExtensionStorageMonitorFactory();
  ~ExtensionStorageMonitorFactory() override;

  // BrowserContextKeyedServiceFactory implementation:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_STORAGE_MONITOR_FACTORY_H_

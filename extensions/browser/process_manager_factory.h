// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_PROCESS_MANAGER_FACTORY_H_
#define EXTENSIONS_BROWSER_PROCESS_MANAGER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace extensions {

class ProcessManager;

class ProcessManagerFactory : public BrowserContextKeyedServiceFactory {
 public:
  static ProcessManager* GetForBrowserContext(content::BrowserContext* context);
  // Returns NULL if there is no ProcessManager associated with this context.
  static ProcessManager* GetForBrowserContextIfExists(
      content::BrowserContext* context);
  static ProcessManagerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<ProcessManagerFactory>;

  ProcessManagerFactory();
  ~ProcessManagerFactory() override;

  // BrowserContextKeyedServiceFactory
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(ProcessManagerFactory);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_PROCESS_MANAGER_FACTORY_H_

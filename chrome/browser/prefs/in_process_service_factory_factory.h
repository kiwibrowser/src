// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFS_IN_PROCESS_SERVICE_FACTORY_FACTORY_H_
#define CHROME_BROWSER_PREFS_IN_PROCESS_SERVICE_FACTORY_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace prefs {
class InProcessPrefServiceFactory;
}

class InProcessPrefServiceFactoryFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static InProcessPrefServiceFactoryFactory* GetInstance();

  static prefs::InProcessPrefServiceFactory* GetInstanceForContext(
      content::BrowserContext* context);

 private:
  friend struct base::DefaultSingletonTraits<
      InProcessPrefServiceFactoryFactory>;

  InProcessPrefServiceFactoryFactory();
  ~InProcessPrefServiceFactoryFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(InProcessPrefServiceFactoryFactory);
};

#endif  // CHROME_BROWSER_PREFS_IN_PROCESS_SERVICE_FACTORY_FACTORY_H_

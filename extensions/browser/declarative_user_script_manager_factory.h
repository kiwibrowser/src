// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_DECLARATIVE_USER_SCRIPT_MANAGER_FACTORY_H_
#define EXTENSIONS_BROWSER_DECLARATIVE_USER_SCRIPT_MANAGER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace extensions {

class DeclarativeUserScriptManager;

class DeclarativeUserScriptManagerFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static DeclarativeUserScriptManager* GetForBrowserContext(
      content::BrowserContext* context);
  static DeclarativeUserScriptManagerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      DeclarativeUserScriptManagerFactory>;

  DeclarativeUserScriptManagerFactory();
  ~DeclarativeUserScriptManagerFactory() override;

  // BrowserContextKeyedServiceFactory implementation
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(DeclarativeUserScriptManagerFactory);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_DECLARATIVE_USER_SCRIPT_MANAGER_FACTORY_H_

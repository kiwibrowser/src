// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_BLACKLIST_FACTORY_H_
#define CHROME_BROWSER_EXTENSIONS_BLACKLIST_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace extensions {

class Blacklist;

class BlacklistFactory : public BrowserContextKeyedServiceFactory {
 public:
  static Blacklist* GetForBrowserContext(content::BrowserContext* context);
  static BlacklistFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BlacklistFactory>;

  BlacklistFactory();
  ~BlacklistFactory() override;

  // BrowserContextKeyedServiceFactory
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(BlacklistFactory);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_BLACKLIST_FACTORY_H_

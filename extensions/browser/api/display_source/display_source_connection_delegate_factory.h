// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DISPLAY_SOURCE_DISPLAY_SOURCE_CONNECTION_DELEGATE_FACTORY_H_
#define EXTENSIONS_BROWSER_API_DISPLAY_SOURCE_DISPLAY_SOURCE_CONNECTION_DELEGATE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "extensions/browser/api/display_source/display_source_connection_delegate.h"

namespace context {
class BrowserContext;
}

namespace extensions {

class DisplaySourceConnectionDelegateFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static DisplaySourceConnectionDelegate* GetForBrowserContext(
      content::BrowserContext* browser_context);
  static DisplaySourceConnectionDelegateFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      DisplaySourceConnectionDelegateFactory>;

  DisplaySourceConnectionDelegateFactory();
  ~DisplaySourceConnectionDelegateFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* browser_context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(DisplaySourceConnectionDelegateFactory);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DISPLAY_SOURCE_DISPLAY_SOURCE_CONNECTION_DELEGATE_FACTORY_H_

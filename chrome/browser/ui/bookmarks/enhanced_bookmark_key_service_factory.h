// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BOOKMARKS_ENHANCED_BOOKMARK_KEY_SERVICE_FACTORY_H_
#define CHROME_BROWSER_UI_BOOKMARKS_ENHANCED_BOOKMARK_KEY_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

// Singleton that owns all EnhancedBookmarkKeyServices and associates them with
// BrowserContexts.
class EnhancedBookmarkKeyServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static EnhancedBookmarkKeyServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<EnhancedBookmarkKeyServiceFactory>;

  EnhancedBookmarkKeyServiceFactory();
  ~EnhancedBookmarkKeyServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(EnhancedBookmarkKeyServiceFactory);
};

#endif  // CHROME_BROWSER_UI_BOOKMARKS_ENHANCED_BOOKMARK_KEY_SERVICE_FACTORY_H_

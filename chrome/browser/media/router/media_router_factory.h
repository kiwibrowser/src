// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_FACTORY_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_FACTORY_H_

#include "base/gtest_prod_util.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}

namespace media_router {

class MediaRouter;

// A factory that lazily returns a MediaRouter implementation for a given
// BrowserContext.
class MediaRouterFactory : public BrowserContextKeyedServiceFactory {
 public:
  static MediaRouter* GetApiForBrowserContext(content::BrowserContext* context);

  static MediaRouterFactory* GetInstance();

 protected:
  // We override the shutdown method for the factory to give the Media Router a
  // chance to remove incognito media routes.
  void BrowserContextShutdown(content::BrowserContext* context) override;

 private:
  friend struct base::LazyInstanceTraitsBase<MediaRouterFactory>;
  FRIEND_TEST_ALL_PREFIXES(MediaRouterFactoryTest,
                           IncognitoBrowserContextShutdown);

  MediaRouterFactory();
  ~MediaRouterFactory() override;

  // BrowserContextKeyedServiceFactory interface.
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterFactory);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTER_FACTORY_H_

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_COMMON_WEBSERVICE_CACHE_FACTORY_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_COMMON_WEBSERVICE_CACHE_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template<typename T> struct DefaultSingletonTraits;
}

namespace content {
class BrowserContext;
}

namespace app_list {

class WebserviceCache;

// Singleton that owns the WebserviceCaches and associates them with profiles;
class WebserviceCacheFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns singleton instance of WebserviceCacheFactory.
  static WebserviceCacheFactory* GetInstance();

  // Returns the Webservice cache associated with |context|.
  static WebserviceCache* GetForBrowserContext(
      content::BrowserContext* context);

 private:
  friend struct base::DefaultSingletonTraits<WebserviceCacheFactory>;

  WebserviceCacheFactory();
  ~WebserviceCacheFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(WebserviceCacheFactory);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_COMMON_WEBSERVICE_CACHE_FACTORY_H_

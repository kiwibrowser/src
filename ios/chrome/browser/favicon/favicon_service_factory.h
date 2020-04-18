// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_FAVICON_FAVICON_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_FAVICON_FAVICON_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

enum class ServiceAccessType;

namespace favicon {
class FaviconService;
}

namespace ios {

class ChromeBrowserState;

// Singleton that owns all FaviconServices and associates them with
// ios::ChromeBrowserState.
class FaviconServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static favicon::FaviconService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state,
      ServiceAccessType access_type);
  static FaviconServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<FaviconServiceFactory>;

  FaviconServiceFactory();
  ~FaviconServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(FaviconServiceFactory);
};

}  // namespace ios

#endif  // IOS_CHROME_BROWSER_FAVICON_FAVICON_SERVICE_FACTORY_H_

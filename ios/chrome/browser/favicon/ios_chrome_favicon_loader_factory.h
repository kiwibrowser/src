// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_FAVICON_IOS_CHROME_FAVICON_LOADER_FACTORY_H_
#define IOS_CHROME_BROWSER_FAVICON_IOS_CHROME_FAVICON_LOADER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class FaviconLoader;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace ios {
class ChromeBrowserState;
}

// Singleton that owns all FaviconLoaders and associates them with
// ios::ChromeBrowserState.
class IOSChromeFaviconLoaderFactory : public BrowserStateKeyedServiceFactory {
 public:
  static FaviconLoader* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static FaviconLoader* GetForBrowserStateIfExists(
      ios::ChromeBrowserState* browser_state);
  static IOSChromeFaviconLoaderFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<IOSChromeFaviconLoaderFactory>;

  IOSChromeFaviconLoaderFactory();
  ~IOSChromeFaviconLoaderFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(IOSChromeFaviconLoaderFactory);
};

#endif  // IOS_CHROME_BROWSER_FAVICON_IOS_CHROME_FAVICON_LOADER_FACTORY_H_

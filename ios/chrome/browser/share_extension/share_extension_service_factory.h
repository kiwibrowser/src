// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SHARE_EXTENSION_SHARE_EXTENSION_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_SHARE_EXTENSION_SHARE_EXTENSION_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class ShareExtensionService;

namespace ios {
class ChromeBrowserState;
}

// Singleton that creates the ShareExtensionService and associates that service
// with ios::ChromeBrowserState.
class ShareExtensionServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static ShareExtensionService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static ShareExtensionService* GetForBrowserStateIfExists(
      ios::ChromeBrowserState* browser_state);
  static ShareExtensionServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<ShareExtensionServiceFactory>;

  ShareExtensionServiceFactory();
  ~ShareExtensionServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(ShareExtensionServiceFactory);
};

#endif  // IOS_CHROME_BROWSER_SHARE_EXTENSION_SHARE_EXTENSION_SERVICE_FACTORY_H_

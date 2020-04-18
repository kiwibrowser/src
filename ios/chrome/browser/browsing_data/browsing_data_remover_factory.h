// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_REMOVER_FACTORY_H_
#define IOS_CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_REMOVER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace ios {
class ChromeBrowserState;
}

class BrowsingDataRemover;

// Singleton that owns all BrowsingDataRemovers and associates them with
// ios::ChromeBrowserState.
class BrowsingDataRemoverFactory : public BrowserStateKeyedServiceFactory {
 public:
  static BrowsingDataRemover* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static BrowsingDataRemover* GetForBrowserStateIfExists(
      ios::ChromeBrowserState* browser_state);
  static BrowsingDataRemoverFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BrowsingDataRemoverFactory>;

  BrowsingDataRemoverFactory();
  ~BrowsingDataRemoverFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(BrowsingDataRemoverFactory);
};

#endif  // IOS_CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_REMOVER_FACTORY_H_

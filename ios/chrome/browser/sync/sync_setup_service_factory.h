// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SYNC_SYNC_SETUP_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_SYNC_SYNC_SETUP_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

class SyncSetupService;

namespace ios {
class ChromeBrowserState;
}

// Singleton that owns all SyncSetupServices and associates them with
// BrowserStates.
class SyncSetupServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static SyncSetupService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static SyncSetupService* GetForBrowserStateIfExists(
      ios::ChromeBrowserState* browser_state);

  static SyncSetupServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<SyncSetupServiceFactory>;

  SyncSetupServiceFactory();
  ~SyncSetupServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;

  DISALLOW_COPY_AND_ASSIGN(SyncSetupServiceFactory);
};

#endif  // IOS_CHROME_BROWSER_SYNC_SYNC_SETUP_SERVICE_FACTORY_H_

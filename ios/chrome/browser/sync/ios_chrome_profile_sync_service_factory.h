// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SYNC_IOS_CHROME_PROFILE_SYNC_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_SYNC_IOS_CHROME_PROFILE_SYNC_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace browser_sync {
class ProfileSyncService;
}  // namespace browser_sync

namespace ios {
class ChromeBrowserState;
}  // namespace ios

// Singleton that owns all ProfileSyncService and associates them with
// ios::ChromeBrowserState.
class IOSChromeProfileSyncServiceFactory
    : public BrowserStateKeyedServiceFactory {
 public:
  static browser_sync::ProfileSyncService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);

  static browser_sync::ProfileSyncService* GetForBrowserStateIfExists(
      ios::ChromeBrowserState* browser_state);

  static IOSChromeProfileSyncServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      IOSChromeProfileSyncServiceFactory>;

  IOSChromeProfileSyncServiceFactory();
  ~IOSChromeProfileSyncServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
};

#endif  // IOS_CHROME_BROWSER_SYNC_IOS_CHROME_PROFILE_SYNC_SERVICE_FACTORY_H_

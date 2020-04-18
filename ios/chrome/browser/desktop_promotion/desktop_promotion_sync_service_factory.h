// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_DESKTOP_PROMOTION_SYNC_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_SIGNIN_DESKTOP_PROMOTION_SYNC_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace ios {
class ChromeBrowserState;
}

class DesktopPromotionSyncService;

// Singleton that owns all DesktopPromotionSyncService and associates them with
// ios::ChromeBrowserState.
class DesktopPromotionSyncServiceFactory
    : public BrowserStateKeyedServiceFactory {
 public:
  static DesktopPromotionSyncService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);
  static DesktopPromotionSyncServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      DesktopPromotionSyncServiceFactory>;

  DesktopPromotionSyncServiceFactory();
  ~DesktopPromotionSyncServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(DesktopPromotionSyncServiceFactory);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_DESKTOP_PROMOTION_SYNC_SERVICE_FACTORY_H_

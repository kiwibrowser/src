// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DESKTOP_PROMOTION_DESKTOP_PROMOTION_SYNC_SERVICE_H
#define CHROME_BROWSER_DESKTOP_PROMOTION_DESKTOP_PROMOTION_SYNC_SERVICE_H

#include "base/macros.h"
#include "components/browser_sync/profile_sync_service.h"
#include "ios/chrome/browser/desktop_promotion/desktop_promotion_sync_observer.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

// This class is responsible for creating a DesktopPromotionSyncObserver
// after the main browser state is initialized.
// An object from this class should only be created by
// DesktopPromotionSyncServiceFactory.
class DesktopPromotionSyncService : public KeyedService {
 public:
  // Only the DesktopPromotionSyncServiceFactory and tests should call this.
  DesktopPromotionSyncService(PrefService* pref_service,
                              browser_sync::ProfileSyncService* sync_service);

  ~DesktopPromotionSyncService() override;

  // Register profile specific desktop promotion related preferences.
  static void RegisterDesktopPromotionUserPrefs(
      user_prefs::PrefRegistrySyncable* registry);

 private:
  DesktopPromotionSyncObserver observer_;
};

#endif  // CHROME_BROWSER_DESKTOP_PROMOTION_DESKTOP_PROMOTION_SYNC_SERVICE_H

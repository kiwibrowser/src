// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_DESKTOP_PROMOTION_SYNC_OBSERVER_H_
#define IOS_CHROME_BROWSER_SIGNIN_DESKTOP_PROMOTION_SYNC_OBSERVER_H_

#include "base/macros.h"
#include "components/sync/driver/sync_service_observer.h"

namespace browser_sync {
class ProfileSyncService;
}

class PrefService;

// This class will implement ProfileSyncServiceObserver and will attach it self
// to the sync service to observe the sync state change.
// Once the sync state is changed and priority prefs are synced, the observer
// will check the desktop promotion prefs, and if eligilble it will log desktop
// promotion metrics to uma and mark the promotion cycle as completed in a perf.
class DesktopPromotionSyncObserver : public syncer::SyncServiceObserver {
 public:
  DesktopPromotionSyncObserver(PrefService* pref_service,
                               browser_sync::ProfileSyncService* sync_service);

  ~DesktopPromotionSyncObserver() override;

  // ProfileSyncServiceObserver implementation.
  void OnStateChanged(syncer::SyncService* sync) override;

 private:
  PrefService* pref_service_;
  browser_sync::ProfileSyncService* sync_service_;
  bool desktop_metrics_logger_initiated_;

  DISALLOW_COPY_AND_ASSIGN(DesktopPromotionSyncObserver);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_DESKTOP_PROMOTION_SYNC_OBSERVER_H_

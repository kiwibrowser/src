// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_NETWORK_DATA_PROMO_NOTIFICATION_H_
#define CHROME_BROWSER_UI_ASH_NETWORK_DATA_PROMO_NOTIFICATION_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/network/network_state_handler_observer.h"

class PrefRegistrySimple;

// This class is responsible for triggering cellular network related
// notifications, specifically:
// * "Chrome will use mobile data..." when Cellular is the Default network
//   for the first time.
// * Data Promotion notifications when available / appropriate.
class DataPromoNotification : public chromeos::NetworkStateHandlerObserver {
 public:
  DataPromoNotification();
  ~DataPromoNotification() override;

  static void RegisterPrefs(PrefRegistrySimple* registry);

 private:
  // NetworkStateHandlerObserver
  void NetworkPropertiesUpdated(const chromeos::NetworkState* network) override;
  void DefaultNetworkChanged(const chromeos::NetworkState* network) override;

  // Shows 3G promo notification if needed.
  void ShowOptionalMobileDataPromoNotification();

  // Show notification prompting user to install Data Saver extension.
  bool ShowDataSaverNotification();

  // True if we've shown notifications during this session, or won't need to.
  bool notifications_shown_;

  // Factory for delaying showing promo notification.
  base::WeakPtrFactory<DataPromoNotification> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DataPromoNotification);
};

#endif  // CHROME_BROWSER_UI_ASH_NETWORK_DATA_PROMO_NOTIFICATION_H_

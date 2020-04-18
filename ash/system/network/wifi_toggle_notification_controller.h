// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_NETWORK_WIFI_TOGGLE_NOTIFICATION_CONTROLLER_H_
#define ASH_SYSTEM_NETWORK_WIFI_TOGGLE_NOTIFICATION_CONTROLLER_H_

#include "ash/system/network/network_observer.h"
#include "base/macros.h"

namespace ash {

class WifiToggleNotificationController : public NetworkObserver {
 public:
  WifiToggleNotificationController();
  ~WifiToggleNotificationController() override;

  // NetworkObserver:
  void RequestToggleWifi() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WifiToggleNotificationController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_NETWORK_WIFI_TOGGLE_NOTIFICATION_CONTROLLER_H_

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_NETWORK_TRAY_NETWORK_STATE_OBSERVER_H_
#define ASH_SYSTEM_NETWORK_TRAY_NETWORK_STATE_OBSERVER_H_

#include "base/macros.h"
#include "base/timer/timer.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "chromeos/network/portal_detector/network_portal_detector.h"

namespace ash {

class TrayNetworkStateObserver
    : public chromeos::NetworkStateHandlerObserver,
      public chromeos::NetworkPortalDetector::Observer {
 public:
  class Delegate {
   public:
    // Called when any interesting network changes occur. The frequency of this
    // event is limited to kUpdateFrequencyMs.
    virtual void NetworkStateChanged(bool notify_a11y) = 0;

   protected:
    virtual ~Delegate() {}
  };

  explicit TrayNetworkStateObserver(Delegate* delegate);

  ~TrayNetworkStateObserver() override;

  // NetworkStateHandlerObserver
  void NetworkListChanged() override;
  void DeviceListChanged() override;
  void DefaultNetworkChanged(const chromeos::NetworkState* network) override;
  void NetworkConnectionStateChanged(
      const chromeos::NetworkState* network) override;
  void NetworkPropertiesUpdated(const chromeos::NetworkState* network) override;
  void DevicePropertiesUpdated(const chromeos::DeviceState* device) override;

  // NetworkPortalDetector::Observer
  void OnPortalDetectionCompleted(
      const chromeos::NetworkState* network,
      const chromeos::NetworkPortalDetector::CaptivePortalState& state)
      override;

 private:
  void SignalUpdate(bool notify_a11y);
  void SendNetworkStateChanged(bool notify_a11y);

  // Unowned Delegate pointer (must outlive this instance).
  Delegate* delegate_;

  // Set to true when we should purge stale icons in the cache.
  bool purge_icons_;

  // Frequency at which to push NetworkStateChanged updates. This avoids
  // unnecessarily frequent UI updates (which can be expensive). We set this
  // to 0 for tests to eliminate timing variance.
  int update_frequency_;

  // Timer used to limit the frequency of NetworkStateChanged updates.
  base::OneShotTimer timer_;

  // The previous state of the wifi network, used to immediately send
  // NetworkStateChanged update when wifi changed from enabled->disabled.
  bool wifi_enabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(TrayNetworkStateObserver);
};

}  // namespace ash

#endif  // ASH_SYSTEM_NETWORK_TRAY_NETWORK_STATE_OBSERVER_H_

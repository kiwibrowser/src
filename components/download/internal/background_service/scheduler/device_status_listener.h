// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_SCHEDULER_DEVICE_STATUS_LISTENER_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_SCHEDULER_DEVICE_STATUS_LISTENER_H_

#include <memory>

#include "base/power_monitor/power_observer.h"
#include "base/timer/timer.h"
#include "components/download/internal/background_service/scheduler/device_status.h"
#include "components/download/internal/background_service/scheduler/network_status_listener.h"
#include "net/base/network_change_notifier.h"

namespace download {

// Helper class to listen to battery changes.
class BatteryStatusListener : public base::PowerObserver {
 public:
  class Observer {
   public:
    // Called when device charging state changed.
    virtual void OnPowerStateChange(bool on_battery_power) = 0;

   protected:
    virtual ~Observer() {}
  };

  BatteryStatusListener(const base::TimeDelta& battery_query_interval);
  ~BatteryStatusListener() override;

  int GetBatteryPercentage();
  bool IsOnBatteryPower();

  void Start(Observer* observer);
  void Stop();

 protected:
  // Platform specific code should override to query the actual battery state.
  virtual int GetBatteryPercentageInternal();

 private:
  // Updates battery percentage. Will throttle based on
  // |battery_query_interval_| when |force| is false.
  void UpdateBatteryPercentage(bool force);

  // base::PowerObserver implementation.
  void OnPowerStateChange(bool on_battery_power) override;

  // Cached battery percentage.
  int battery_percentage_;

  // Interval to throttle battery queries. Cached value will be returned inside
  // this interval.
  base::TimeDelta battery_query_interval_;

  // Time stamp to record last battery query.
  base::Time last_battery_query_;

  Observer* observer_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(BatteryStatusListener);
};

// Listens to network and battery status change and notifies the observer.
class DeviceStatusListener : public NetworkStatusListener::Observer,
                             public BatteryStatusListener::Observer {
 public:
  class Observer {
   public:
    // Called when device status is changed.
    virtual void OnDeviceStatusChanged(const DeviceStatus& device_status) = 0;
  };

  explicit DeviceStatusListener(
      const base::TimeDelta& startup_delay,
      const base::TimeDelta& online_delay,
      std::unique_ptr<BatteryStatusListener> battery_listener);
  ~DeviceStatusListener() override;

  bool is_valid_state() { return is_valid_state_; }

  // Returns the current device status for download scheduling. May update
  // internal device status when called.
  const DeviceStatus& CurrentDeviceStatus();

  // Starts/stops to listen network and battery change events, virtual for
  // testing.
  virtual void Start(DeviceStatusListener::Observer* observer);
  virtual void Stop();

 protected:
  // Creates the instance of |network_listener_|, visible for testing.
  virtual void BuildNetworkStatusListener();

  // Used to listen to network connectivity changes.
  std::unique_ptr<NetworkStatusListener> network_listener_;

  // The current device status.
  DeviceStatus status_;

  // The observer that listens to device status change events.
  Observer* observer_;

  // If device status listener is started.
  bool listening_;

  bool is_valid_state_;

 private:
  // Start after a delay to wait for potential network stack setup.
  void StartAfterDelay();

  // NetworkStatusListener::Observer implementation.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // BatteryStatusListener::Observer implementation.
  void OnPowerStateChange(bool on_battery_power) override;

  // Notifies the observer about device status change.
  void NotifyStatusChange();

  // Called after a delay to notify the observer. See |delay_|.
  void NotifyNetworkChange();

  // Used to start the device listener or notify network change after a delay.
  base::OneShotTimer timer_;

  // The delay used on start up.
  base::TimeDelta startup_delay_;

  // The delay used when network status becomes online.
  base::TimeDelta online_delay_;

  // Pending network status used to update the current network status.
  NetworkStatus pending_network_status_ = NetworkStatus::DISCONNECTED;

  // Used to listen to battery status.
  std::unique_ptr<BatteryStatusListener> battery_listener_;

  DISALLOW_COPY_AND_ASSIGN(DeviceStatusListener);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_SCHEDULER_DEVICE_STATUS_LISTENER_H_

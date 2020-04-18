// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/background_service/scheduler/device_status_listener.h"

#include "base/power_monitor/power_monitor.h"
#include "build/build_config.h"
#include "components/download/internal/background_service/scheduler/network_status_listener.h"

#if defined(OS_ANDROID)
#include "components/download/internal/background_service/android/network_status_listener_android.h"
#endif

namespace download {

namespace {

// Converts |on_battery_power| to battery status.
BatteryStatus ToBatteryStatus(bool on_battery_power) {
  return on_battery_power ? BatteryStatus::NOT_CHARGING
                          : BatteryStatus::CHARGING;
}

// Converts a ConnectionType to NetworkStatus.
NetworkStatus ToNetworkStatus(net::NetworkChangeNotifier::ConnectionType type) {
  switch (type) {
    case net::NetworkChangeNotifier::CONNECTION_ETHERNET:
    case net::NetworkChangeNotifier::CONNECTION_WIFI:
      return NetworkStatus::UNMETERED;
    case net::NetworkChangeNotifier::CONNECTION_2G:
    case net::NetworkChangeNotifier::CONNECTION_3G:
    case net::NetworkChangeNotifier::CONNECTION_4G:
      return NetworkStatus::METERED;
    case net::NetworkChangeNotifier::CONNECTION_UNKNOWN:
    case net::NetworkChangeNotifier::CONNECTION_NONE:
    case net::NetworkChangeNotifier::CONNECTION_BLUETOOTH:
      return NetworkStatus::DISCONNECTED;
  }
  NOTREACHED();
  return NetworkStatus::DISCONNECTED;
}

}  // namespace

BatteryStatusListener::BatteryStatusListener(
    const base::TimeDelta& battery_query_interval)
    : battery_percentage_(0),
      battery_query_interval_(battery_query_interval),
      last_battery_query_(base::Time::Now()) {}

BatteryStatusListener::~BatteryStatusListener() = default;

int BatteryStatusListener::GetBatteryPercentage() {
  UpdateBatteryPercentage(false);
  return battery_percentage_;
}

bool BatteryStatusListener::IsOnBatteryPower() {
  return base::PowerMonitor::Get()->IsOnBatteryPower();
}

void BatteryStatusListener::Start(Observer* observer) {
  observer_ = observer;

  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  DCHECK(power_monitor);
  power_monitor->AddObserver(this);

  UpdateBatteryPercentage(true);
}

void BatteryStatusListener::Stop() {
  base::PowerMonitor::Get()->RemoveObserver(this);
}

int BatteryStatusListener::GetBatteryPercentageInternal() {
  // Non-Android implementation currently always return full battery.
  return 100;
}

void BatteryStatusListener::UpdateBatteryPercentage(bool force) {
  // Throttle the battery queries.
  if (!force &&
      base::Time::Now() - last_battery_query_ < battery_query_interval_)
    return;

  battery_percentage_ = GetBatteryPercentageInternal();
  last_battery_query_ = base::Time::Now();
}

void BatteryStatusListener::OnPowerStateChange(bool on_battery_power) {
  if (observer_)
    observer_->OnPowerStateChange(on_battery_power);
}

DeviceStatusListener::DeviceStatusListener(
    const base::TimeDelta& startup_delay,
    const base::TimeDelta& online_delay,
    std::unique_ptr<BatteryStatusListener> battery_listener)
    : observer_(nullptr),
      listening_(false),
      is_valid_state_(false),
      startup_delay_(startup_delay),
      online_delay_(online_delay),
      battery_listener_(std::move(battery_listener)) {}

DeviceStatusListener::~DeviceStatusListener() {
  Stop();
}

const DeviceStatus& DeviceStatusListener::CurrentDeviceStatus() {
  DCHECK(battery_listener_);
  status_.battery_percentage = battery_listener_->GetBatteryPercentage();
  return status_;
}

void DeviceStatusListener::Start(DeviceStatusListener::Observer* observer) {
  if (listening_)
    return;

  DCHECK(observer);
  observer_ = observer;

  // Network stack may shake off all connections after getting the IP address,
  // use a delay to wait for potential network setup.
  timer_.Start(FROM_HERE, startup_delay_,
               base::Bind(&DeviceStatusListener::StartAfterDelay,
                          base::Unretained(this)));
}

void DeviceStatusListener::StartAfterDelay() {
  // Listen to battery status changes.
  DCHECK(battery_listener_);
  battery_listener_->Start(this);
  status_.battery_status =
      ToBatteryStatus(battery_listener_->IsOnBatteryPower());

  // Listen to network status changes.
  BuildNetworkStatusListener();
  network_listener_->Start(this);

  status_.battery_status =
      ToBatteryStatus(base::PowerMonitor::Get()->IsOnBatteryPower());
  status_.network_status =
      ToNetworkStatus(network_listener_->GetConnectionType());
  pending_network_status_ = status_.network_status;

  listening_ = true;
  is_valid_state_ = true;

  NotifyStatusChange();
}

void DeviceStatusListener::Stop() {
  timer_.Stop();

  if (!listening_)
    return;

  battery_listener_->Stop();
  battery_listener_.reset();

  network_listener_->Stop();
  network_listener_.reset();

  status_ = DeviceStatus();
  listening_ = false;
  observer_ = nullptr;
}

void DeviceStatusListener::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  pending_network_status_ = ToNetworkStatus(type);

  if (pending_network_status_ == status_.network_status) {
    timer_.Stop();
    is_valid_state_ = true;
    return;
  }

  bool change_to_online =
      (status_.network_status == NetworkStatus::DISCONNECTED) &&
      (pending_network_status_ != NetworkStatus::DISCONNECTED);

  // It's unreliable to send requests immediately after the network becomes
  // online that the signal may not fully consider DHCP. Notify network change
  // to the observer after a delay.
  // Android network change listener still need this delay.
  if (change_to_online) {
    is_valid_state_ = false;
    timer_.Start(FROM_HERE, online_delay_,
                 base::Bind(&DeviceStatusListener::NotifyNetworkChange,
                            base::Unretained(this)));
  } else {
    timer_.Stop();
    NotifyNetworkChange();
  }
}

void DeviceStatusListener::OnPowerStateChange(bool on_battery_power) {
  status_.battery_status = ToBatteryStatus(on_battery_power);
  NotifyStatusChange();
}

void DeviceStatusListener::NotifyStatusChange() {
  DCHECK(observer_);
  observer_->OnDeviceStatusChanged(status_);
}

void DeviceStatusListener::NotifyNetworkChange() {
  is_valid_state_ = true;
  if (pending_network_status_ == status_.network_status)
    return;

  status_.network_status = pending_network_status_;
  NotifyStatusChange();
}

void DeviceStatusListener::BuildNetworkStatusListener() {
#if defined(OS_ANDROID)
  network_listener_ = std::make_unique<NetworkStatusListenerAndroid>();
#else
  network_listener_ = std::make_unique<NetworkStatusListenerImpl>();
#endif
}

}  // namespace download

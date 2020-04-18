// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/power_monitor/power_monitor_message_broadcaster.h"

#include "base/power_monitor/power_monitor.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace device {

PowerMonitorMessageBroadcaster::PowerMonitorMessageBroadcaster() {
  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor)
    power_monitor->AddObserver(this);
}

PowerMonitorMessageBroadcaster::~PowerMonitorMessageBroadcaster() {
  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor)
    power_monitor->RemoveObserver(this);
}

// static
void PowerMonitorMessageBroadcaster::Bind(
    device::mojom::PowerMonitorRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void PowerMonitorMessageBroadcaster::AddClient(
    device::mojom::PowerMonitorClientPtr power_monitor_client) {
  clients_.AddPtr(std::move(power_monitor_client));
  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  // Unit tests does not initialize the PowerMonitor.
  if (power_monitor) {
    OnPowerStateChange(power_monitor->IsOnBatteryPower());
  }
}

void PowerMonitorMessageBroadcaster::OnPowerStateChange(bool on_battery_power) {
  clients_.ForAllPtrs([&on_battery_power](mojom::PowerMonitorClient* client) {
    client->PowerStateChange(on_battery_power);
  });
}

void PowerMonitorMessageBroadcaster::OnSuspend() {
  clients_.ForAllPtrs(
      [](mojom::PowerMonitorClient* client) { client->Suspend(); });
}

void PowerMonitorMessageBroadcaster::OnResume() {
  clients_.ForAllPtrs(
      [](mojom::PowerMonitorClient* client) { client->Resume(); });
}

}  // namespace device

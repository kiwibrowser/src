// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/common/power_monitor_test_impl.h"

#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

// static
void PowerMonitorTestImpl::MakeStrongBinding(
    std::unique_ptr<PowerMonitorTestImpl> instance,
    mojom::PowerMonitorTestRequest request) {
  mojo::MakeStrongBinding(std::move(instance), std::move(request));
}

PowerMonitorTestImpl::PowerMonitorTestImpl() {
  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor)
    power_monitor->AddObserver(this);
}

PowerMonitorTestImpl::~PowerMonitorTestImpl() {
  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  if (power_monitor)
    power_monitor->RemoveObserver(this);
}

void PowerMonitorTestImpl::QueryNextState(QueryNextStateCallback callback) {
  // Do not allow overlapping call.
  DCHECK(callback_.is_null());
  callback_ = std::move(callback);

  if (need_to_report_)
    ReportState();
}

void PowerMonitorTestImpl::OnPowerStateChange(bool on_battery_power) {
  on_battery_power_ = on_battery_power;
  need_to_report_ = true;

  if (!callback_.is_null())
    ReportState();
}

void PowerMonitorTestImpl::ReportState() {
  std::move(callback_).Run(on_battery_power_);
  need_to_report_ = false;
}

}  // namespace content

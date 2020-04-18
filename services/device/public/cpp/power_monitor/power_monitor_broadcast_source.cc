// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/public/cpp/power_monitor/power_monitor_broadcast_source.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace device {

PowerMonitorBroadcastSource::PowerMonitorBroadcastSource(
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : PowerMonitorBroadcastSource(std::make_unique<Client>(),
                                  task_runner) {}

PowerMonitorBroadcastSource::PowerMonitorBroadcastSource(
    std::unique_ptr<Client> client,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : client_(std::move(client)), task_runner_(task_runner) {}

PowerMonitorBroadcastSource::~PowerMonitorBroadcastSource() {
  // When power monitor and source are destroyed, the IO thread could still be
  // receiving mojo messages that access the monitor and source in
  // |ProcessPowerEvent|, thus causing a data race. Calling Shutdown() on the
  // client tells it to ignore future mojo messages, and thus prevents the data
  // race.
  client_->Shutdown();
  task_runner_->DeleteSoon(FROM_HERE, client_.release());
}

void PowerMonitorBroadcastSource::Init(service_manager::Connector* connector) {
  if (connector) {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&PowerMonitorBroadcastSource::Client::Init,
                                  base::Unretained(client_.get()),
                                  base::Passed(connector->Clone())));
  }
}

bool PowerMonitorBroadcastSource::IsOnBatteryPowerImpl() {
  return client_->last_reported_on_battery_power_state();
}

PowerMonitorBroadcastSource::Client::Client() : binding_(this) {}

PowerMonitorBroadcastSource::Client::~Client() {}

void PowerMonitorBroadcastSource::Client::Init(
    std::unique_ptr<service_manager::Connector> connector) {
  base::AutoLock auto_lock(is_shutdown_lock_);
  if (is_shutdown_)
    return;
  connector_ = std::move(connector);
  device::mojom::PowerMonitorPtr power_monitor;
  connector_->BindInterface(device::mojom::kServiceName,
                            mojo::MakeRequest(&power_monitor));
  device::mojom::PowerMonitorClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  power_monitor->AddClient(std::move(client));
}

void PowerMonitorBroadcastSource::Client::Shutdown() {
  base::AutoLock auto_lock(is_shutdown_lock_);
  DCHECK(!is_shutdown_);
  is_shutdown_ = true;
}

void PowerMonitorBroadcastSource::Client::PowerStateChange(
    bool on_battery_power) {
  base::AutoLock auto_lock(is_shutdown_lock_);
  if (is_shutdown_)
    return;
  last_reported_on_battery_power_state_ = on_battery_power;
  ProcessPowerEvent(PowerMonitorSource::POWER_STATE_EVENT);
}

void PowerMonitorBroadcastSource::Client::Suspend() {
  base::AutoLock auto_lock(is_shutdown_lock_);
  if (is_shutdown_)
    return;
  ProcessPowerEvent(PowerMonitorSource::SUSPEND_EVENT);
}

void PowerMonitorBroadcastSource::Client::Resume() {
  base::AutoLock auto_lock(is_shutdown_lock_);
  if (is_shutdown_)
    return;
  ProcessPowerEvent(PowerMonitorSource::RESUME_EVENT);
}

}  // namespace device

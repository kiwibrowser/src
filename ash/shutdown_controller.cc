// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shutdown_controller.h"

#include <utility>

#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/shutdown_reason.h"
#include "ash/wm/lock_state_controller.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager_client.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace ash {

ShutdownController::ShutdownController() = default;

ShutdownController::~ShutdownController() = default;

void ShutdownController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ShutdownController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void ShutdownController::ShutDownOrReboot(ShutdownReason reason) {
  // For developers on Linux desktop just exit the app.
  if (!base::SysInfo::IsRunningOnChromeOS()) {
    Shell::Get()->session_controller()->RequestSignOut();
    return;
  }

  if (reason == ShutdownReason::POWER_BUTTON)
    base::RecordAction(base::UserMetricsAction("Accel_ShutDown_PowerButton"));

  // On real Chrome OS hardware the power manager handles shutdown.
  using chromeos::DBusThreadManager;
  std::string description = base::StringPrintf("UI request from ash: %s",
                                               ShutdownReasonToString(reason));
  if (reboot_on_shutdown_) {
    DBusThreadManager::Get()->GetPowerManagerClient()->RequestRestart(
        power_manager::REQUEST_RESTART_FOR_USER, description);
  } else {
    DBusThreadManager::Get()->GetPowerManagerClient()->RequestShutdown(
        power_manager::REQUEST_SHUTDOWN_FOR_USER, description);
  }
}

void ShutdownController::BindRequest(mojom::ShutdownControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void ShutdownController::SetRebootOnShutdownForTesting(
    bool reboot_on_shutdown) {
  SetRebootOnShutdown(reboot_on_shutdown);
}

void ShutdownController::SetRebootOnShutdown(bool reboot_on_shutdown) {
  if (reboot_on_shutdown_ == reboot_on_shutdown)
    return;
  reboot_on_shutdown_ = reboot_on_shutdown;
  for (auto& observer : observers_)
    observer.OnShutdownPolicyChanged(reboot_on_shutdown_);
}

void ShutdownController::RequestShutdownFromLoginScreen() {
  Shell::Get()->lock_state_controller()->RequestShutdown(
      ShutdownReason::LOGIN_SHUT_DOWN_BUTTON);
}

}  // namespace ash

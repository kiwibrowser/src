// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/remote_commands/device_commands_factory_chromeos.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/chromeos/policy/remote_commands/device_command_fetch_status_job.h"
#include "chrome/browser/chromeos/policy/remote_commands/device_command_reboot_job.h"
#include "chrome/browser/chromeos/policy/remote_commands/device_command_screenshot_job.h"
#include "chrome/browser/chromeos/policy/remote_commands/device_command_set_volume_job.h"
#include "chrome/browser/chromeos/policy/remote_commands/screenshot_delegate.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "components/policy/core/common/remote_commands/remote_command_job.h"
#include "components/policy/proto/device_management_backend.pb.h"

namespace em = enterprise_management;

namespace policy {

DeviceCommandsFactoryChromeOS::DeviceCommandsFactoryChromeOS() {
}

DeviceCommandsFactoryChromeOS::~DeviceCommandsFactoryChromeOS() {
}

std::unique_ptr<RemoteCommandJob>
DeviceCommandsFactoryChromeOS::BuildJobForType(em::RemoteCommand_Type type) {
  switch (type) {
    case em::RemoteCommand_Type_DEVICE_REBOOT:
      return base::WrapUnique<RemoteCommandJob>(new DeviceCommandRebootJob(
          chromeos::DBusThreadManager::Get()->GetPowerManagerClient()));
    case em::RemoteCommand_Type_DEVICE_SCREENSHOT:
      return base::WrapUnique<RemoteCommandJob>(new DeviceCommandScreenshotJob(
          std::make_unique<ScreenshotDelegate>()));
    case em::RemoteCommand_Type_DEVICE_SET_VOLUME:
      return base::WrapUnique<RemoteCommandJob>(
          new DeviceCommandSetVolumeJob());
    case em::RemoteCommand_Type_DEVICE_FETCH_STATUS:
      return base::WrapUnique<RemoteCommandJob>(
          new DeviceCommandFetchStatusJob());
    default:
      // Other types of commands should be sent to UserCommandsFactoryChromeOS
      // instead of here.
      NOTREACHED();
      return nullptr;
  }
}

}  // namespace policy

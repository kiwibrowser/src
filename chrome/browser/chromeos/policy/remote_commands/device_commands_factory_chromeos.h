// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_REMOTE_COMMANDS_DEVICE_COMMANDS_FACTORY_CHROMEOS_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_REMOTE_COMMANDS_DEVICE_COMMANDS_FACTORY_CHROMEOS_H_

#include <memory>

#include "base/macros.h"
#include "components/policy/core/common/remote_commands/remote_commands_factory.h"

namespace policy {

class DeviceCommandsFactoryChromeOS : public RemoteCommandsFactory {
 public:
  DeviceCommandsFactoryChromeOS();
  ~DeviceCommandsFactoryChromeOS() override;

  // RemoteCommandsFactory:
  std::unique_ptr<RemoteCommandJob> BuildJobForType(
      enterprise_management::RemoteCommand_Type type) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceCommandsFactoryChromeOS);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_REMOTE_COMMANDS_DEVICE_COMMANDS_FACTORY_CHROMEOS_H_

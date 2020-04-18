// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/virtual_machines/virtual_machines_util.h"

#include "base/version.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/common/channel_info.h"
#include "chromeos/settings/cros_settings_names.h"
#include "components/version_info/version_info.h"

namespace virtual_machines {

bool AreVirtualMachinesAllowedByPolicy() {
  bool virtual_machines_allowed;
  if (chromeos::CrosSettings::Get()->GetBoolean(
          chromeos::kVirtualMachinesAllowed, &virtual_machines_allowed)) {
    return virtual_machines_allowed;
  }
  // If device policy is not set, allow virtual machines.
  return true;
}

// Disabled for beta/stable channel for M67 and M68
// and all versions older than M67.
bool AreVirtualMachinesAllowedByVersionAndChannel() {
  const base::Version& current_version = version_info::GetVersion();
  if (!current_version.IsValid())
    return false;
  if (current_version.CompareToWildcardString("67.*") < 0)
    return false;
  if (current_version.CompareToWildcardString("68.*") <= 0) {
    version_info::Channel channel = chrome::GetChannel();
    return channel != version_info::Channel::STABLE &&
           channel != version_info::Channel::BETA;
  }

  return true;
}

}  // namespace virtual_machines

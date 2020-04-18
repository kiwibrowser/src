// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/battor_agent/battor_finder.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "device/serial/serial_device_enumerator.h"
#include "services/device/public/mojom/serial.mojom.h"

namespace battor {

namespace {

// The USB display name prefix that all BattOrs have.
const char kBattOrDisplayNamePrefix[] = "BattOr";

// The command line switch used to hard-code a BattOr path. Hard-coding
// this path disables the normal method of finding a BattOr, which is to
// search through serial devices for one with a matching display name.
const char kBattOrPathSwitch[] = "battor-path";

}  // namespace

std::string BattOrFinder::FindBattOr() {
  std::unique_ptr<device::SerialDeviceEnumerator> serial_device_enumerator =
      device::SerialDeviceEnumerator::Create();

  std::vector<device::mojom::SerialDeviceInfoPtr> devices =
      serial_device_enumerator->GetDevices();

  std::string switch_specified_path =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          kBattOrPathSwitch);
  if (switch_specified_path.empty()) {
    // If we have no switch-specified path, look for a device with the right
    // display name. See crbug.com/588244 for why this never works on Windows.
    for (size_t i = 0; i < devices.size(); i++) {
      if (!devices[i]->display_name)
        continue;
      const auto& display_name = devices[i]->display_name.value();
      if (display_name.find(kBattOrDisplayNamePrefix) != std::string::npos) {
        LOG(INFO) << "Found BattOr with display name " << display_name
                  << " at path " << devices[i]->path;
        return devices[i]->path;
      }
    }
  } else {
    // If we have a switch-specified path, make sure it actually exists before
    // returning it.
    for (size_t i = 0; i < devices.size(); i++) {
      if (devices[i]->path == switch_specified_path)
        return switch_specified_path;
    }
  }

  return std::string();
}

}  // namespace battor

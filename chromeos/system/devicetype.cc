// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/system/devicetype.h"

#include <string>

#include "base/sys_info.h"

namespace chromeos {

namespace {
const char kDeviceType[] = "DEVICETYPE";
}

DeviceType GetDeviceType() {
  std::string value;
  if (base::SysInfo::GetLsbReleaseValue(kDeviceType, &value)) {
    if (value == "CHROMEBASE")
      return DeviceType::kChromebase;
    else if (value == "CHROMEBIT")
      return DeviceType::kChromebit;
    // Most devices are Chromebooks, so we will also consider reference boards
    // as chromebooks.
    else if (value == "CHROMEBOOK" || value == "REFERENCE")
      return DeviceType::kChromebook;
    else if (value == "CHROMEBOX")
      return DeviceType::kChromebox;
    else
      LOG(ERROR) << "Unknown device type \"" << value << "\"";
  }

  return DeviceType::kUnknown;
}

}  // namespace chromeos

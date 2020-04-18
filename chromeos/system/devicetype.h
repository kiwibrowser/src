// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SYSTEM_DEVICETYPE_H_
#define CHROMEOS_SYSTEM_DEVICETYPE_H_

#include "chromeos/chromeos_export.h"

namespace chromeos {

enum class DeviceType {
  kChromebase,
  kChromebit,
  kChromebook,
  kChromebox,
  kUnknown,  // Unknown fallback device.
};

// Returns the current device type, eg, Chromebook, Chromebox.
CHROMEOS_EXPORT DeviceType GetDeviceType();

}  // namespace chromeos

#endif  // CHROMEOS_SYSTEM_DEVICETYPE_H_

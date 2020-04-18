// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CHROMEOS_NETWORK_TETHER_CONSTANTS_H_
#define CHROMEOS_NETWORK_TETHER_CONSTANTS_H_

#include "chromeos/chromeos_export.h"

namespace chromeos {

// This file contains constants for Chrome OS tether networks which are used
// wherever Shill constants are appropriate. Tether networks are a Chrome OS
// concept which does not exist as part of Shill, so these custom definitions
// are used instead. Tether networks are never intended to be passed to Shill
// code, so these constants are used primarily as part of NetworkStateHandler.

// Represents the tether network type.
CHROMEOS_EXPORT extern const char kTypeTether[];

// Properties associated with tether networks.
CHROMEOS_EXPORT extern const char kTetherBatteryPercentage[];
CHROMEOS_EXPORT extern const char kTetherCarrier[];
CHROMEOS_EXPORT extern const char kTetherHasConnectedToHost[];
CHROMEOS_EXPORT extern const char kTetherSignalStrength[];

// The device path used for the tether DeviceState.
CHROMEOS_EXPORT extern const char kTetherDevicePath[];

// The name used for the tether DeviceState.
CHROMEOS_EXPORT extern const char kTetherDeviceName[];

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_TETHER_CONSTANTS_H_

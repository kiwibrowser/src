// Copyright 2018 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_CECSERVICE_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_CECSERVICE_DBUS_CONSTANTS_H_

namespace cecservice {
const char kCecServiceInterface[] = "org.chromium.CecService";
const char kCecServicePath[] = "/org/chromium/CecService";
const char kCecServiceName[] = "org.chromium.CecService";

// Methods.
const char kSendStandByToAllDevicesMethod[] = "SendStandByToAllDevices";
const char kSendWakeUpToAllDevicesMethod[] = "SendWakeUpToAllDevices";
const char kGetTvsPowerStatus[] = "GetTvsPowerStatus";

// Result of a TV power status query.
enum TvPowerStatus {
  kTvPowerStatusError = 0,  // There was an error querying the TV.
  kTvPowerStatusAdapterNotConfigured =
      1,                        // The adapter is not configured (no EDID).
  kTvPowerStatusNoTv = 2,       // There is no TV (the request was not acked).
  kTvPowerStatusOn = 3,         // TV is on.
  kTvPowerStatusStandBy = 4,    // TV is on standby.
  kTvPowerStatusToOn = 5,       // TV transitions to on.
  kTvPowerStatusToStandBy = 6,  // TV transitions to standby.
  kTvPowerStatusUnknown = 7,    // Unknown power status read from TV.
};

}  // namespace cecservice

#endif  // SYSTEM_API_DBUS_CECSERVICE_DBUS_CONSTANTS_H_

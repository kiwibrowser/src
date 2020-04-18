// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_TPM_FIRMWARE_UPDATE_H_
#define CHROME_BROWSER_CHROMEOS_TPM_FIRMWARE_UPDATE_H_

#include <memory>
#include <set>

#include "base/callback_forward.h"
#include "base/time/time.h"

namespace base {
class DictionaryValue;
}

namespace enterprise_management {
class TPMFirmwareUpdateSettingsProto;
}

namespace chromeos {
namespace tpm_firmware_update {

// Constants to identify the TPM firmware update modes that are supported. Do
// not re-assign constants, the numbers appear in local_state pref values.
enum class Mode : int {
  // No update should take place. Used as a default in contexts where there is
  // no proper value.
  kNone = 0,
  // Update TPM firmware via powerwash.
  kPowerwash = 1,
  // Device-state preserving update flow. Destroys all user data.
  kPreserveDeviceState = 2,
};

// Settings dictionary key constants.
extern const char kSettingsKeyAllowPowerwash[];
extern const char kSettingsKeyAllowPreserveDeviceState[];

// Decodes the TPM firmware update settings into base::Value representation.
std::unique_ptr<base::DictionaryValue> DecodeSettingsProto(
    const enterprise_management::TPMFirmwareUpdateSettingsProto& settings);

// Check what update modes are allowed. The |timeout| parameter determines how
// long to wait in case the decision whether an update is available is still
// pending.
void GetAvailableUpdateModes(
    base::OnceCallback<void(const std::set<Mode>&)> completion,
    base::TimeDelta timeout);

}  // namespace tpm_firmware_update
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_TPM_FIRMWARE_UPDATE_H_

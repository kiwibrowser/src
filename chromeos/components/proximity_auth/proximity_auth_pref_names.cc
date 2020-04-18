// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/proximity_auth/proximity_auth_pref_names.h"

namespace proximity_auth {
namespace prefs {

// Whether a user is allowed to use Easy Unlock. This pref is expected to be set
// through an enterprise policy.
const char kEasyUnlockAllowed[] = "easy_unlock.allowed";

// Whether or not EasyUnlock has been enabled by the user (i.e. they went
// through the setup flow and there is at least one unlock key).
const char kEasyUnlockEnabled[] = "easy_unlock.enabled";

// Whether or not the state of EasyUnlock has actively been changed, e.g.,
// explicitly enabled by the user (through setup) or disabled via Settings.
const char kEasyUnlockEnabledStateSet[] = "easy_unlock.enabled_state_set";

// How close the remote and the local device should be in order to use Easy
// Unlock.
const char kEasyUnlockProximityThreshold[] = "easy_unlock.proximity_threshold";

// A dictionary in local state containing each user's Easy Unlock profile
// preferences, so they can be accessed outside of the user's profile. The value
// is a dictionary containing an entry for each user. Each user's entry mirrors
// their profile's Easy Unlock preferences.
const char kEasyUnlockLocalStateUserPrefs[] = "easy_unlock.user_prefs";

// Whether or not EasyUnlock is enabled on the ChromeOS login screen (in
// addition to the lock screen).
const char kProximityAuthIsChromeOSLoginEnabled[] =
    "proximity_auth.is_chromeos_login_enabled";

// The timestamp of the last password entry in milliseconds, used to enforce
// reauthing with the password after a given time period has elapsed.
const char kProximityAuthLastPasswordEntryTimestampMs[] =
    "proximity_auth.last_password_entry_timestamp_ms";

// The timestamp of the last promotion check in milliseconds.
const char kProximityAuthLastPromotionCheckTimestampMs[] =
    "proximity_auth.last_promotion_check_timestamp_ms";

// The number of times the promotion was shown to the user.
const char kProximityAuthPromotionShownCount[] =
    "proximity_auth.promotion_shown_count";

// The dictionary containing remote BLE devices.
const char kProximityAuthRemoteBleDevices[] =
    "proximity_auth.remote_ble_devices";

}  // namespace prefs
}  // namespace proximity_auth

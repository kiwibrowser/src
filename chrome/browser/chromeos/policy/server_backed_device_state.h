// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_SERVER_BACKED_DEVICE_STATE_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_SERVER_BACKED_DEVICE_STATE_H_

namespace policy {

// Dictionary key constants for prefs::kServerBackedDeviceState.
extern const char kDeviceStateManagementDomain[];
extern const char kDeviceStateRestoreMode[];
extern const char kDeviceStateDisabledMessage[];

// String constants used to persist the restorative action in the
// kDeviceStateRestoreMode dictionary entry.
extern const char kDeviceStateRestoreModeReEnrollmentRequested[];
extern const char kDeviceStateRestoreModeReEnrollmentEnforced[];
extern const char kDeviceStateRestoreModeDisabled[];
extern const char kDeviceStateRestoreModeReEnrollmentZeroTouch[];

// Restorative action to take after device reset.
enum RestoreMode {
  // No state restoration.
  RESTORE_MODE_NONE = 0,
  // Enterprise enrollment requested, but user may skip.
  RESTORE_MODE_REENROLLMENT_REQUESTED = 1,
  // Enterprise enrollment is enforced and cannot be skipped.
  RESTORE_MODE_REENROLLMENT_ENFORCED = 2,
  // The device has been disabled by its owner. The device will show a warning
  // screen and prevent the user from proceeding further.
  RESTORE_MODE_DISABLED = 3,
  // Enterprise enrollment is enforced using Zero-Touch and cannot be skipped.
  RESTORE_MODE_REENROLLMENT_ZERO_TOUCH = 4,
};

// Parses the contents of the kDeviceStateRestoreMode dictionary entry and
// returns it as a RestoreMode.
RestoreMode GetRestoreMode();

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_SERVER_BACKED_DEVICE_STATE_H_

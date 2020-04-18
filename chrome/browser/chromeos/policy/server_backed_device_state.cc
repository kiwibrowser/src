// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/server_backed_device_state.h"

#include <string>

#include "base/logging.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_platform_part.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace policy {

const char kDeviceStateManagementDomain[] = "management_domain";
const char kDeviceStateRestoreMode[] = "device_mode";
const char kDeviceStateDisabledMessage[] = "disabled_message";

const char kDeviceStateRestoreModeReEnrollmentRequested[] =
    "re-enrollment-requested";
const char kDeviceStateRestoreModeReEnrollmentEnforced[] =
    "re-enrollment-enforced";
const char kDeviceStateRestoreModeDisabled[] = "disabled";
const char kDeviceStateRestoreModeReEnrollmentZeroTouch[] =
    "re-enrollment-zero-touch";

RestoreMode GetRestoreMode() {
  std::string restore_mode;
  g_browser_process->local_state()->GetDictionary(
      prefs::kServerBackedDeviceState)->GetString(kDeviceStateRestoreMode,
                                                  &restore_mode);
  if (restore_mode.empty())
    return RESTORE_MODE_NONE;
  if (restore_mode == kDeviceStateRestoreModeReEnrollmentRequested)
    return RESTORE_MODE_REENROLLMENT_REQUESTED;
  if (restore_mode == kDeviceStateRestoreModeReEnrollmentEnforced)
    return RESTORE_MODE_REENROLLMENT_ENFORCED;
  if (restore_mode == kDeviceStateRestoreModeDisabled)
    return RESTORE_MODE_DISABLED;
  if (restore_mode == kDeviceStateRestoreModeReEnrollmentZeroTouch)
    return RESTORE_MODE_REENROLLMENT_ZERO_TOUCH;

  NOTREACHED();
  return RESTORE_MODE_NONE;
}

}  // namespace policy

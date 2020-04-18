// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_UPDATE_ENGINE_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_UPDATE_ENGINE_DBUS_CONSTANTS_H_

namespace update_engine {
const char kUpdateEngineInterface[] = "org.chromium.UpdateEngineInterface";
const char kUpdateEngineServicePath[] = "/org/chromium/UpdateEngine";
const char kUpdateEngineServiceName[] = "org.chromium.UpdateEngine";

// Generic UpdateEngine D-Bus error.
const char kUpdateEngineServiceErrorFailed[] =
    "org.chromium.UpdateEngine.Error.Failed";

// Methods.
const char kAttemptUpdate[] = "AttemptUpdate";
const char kGetLastAttemptError[] = "GetLastAttemptError";
const char kGetStatus[] = "GetStatus";
const char kRebootIfNeeded[] = "RebootIfNeeded";
const char kSetChannel[] = "SetChannel";
const char kGetChannel[] = "GetChannel";
const char kAttemptRollback[] = "AttemptRollback";
const char kCanRollback[] = "CanRollback";
const char kGetEolStatus[] = "GetEolStatus";
const char kSetUpdateOverCellularPermission[] =
    "SetUpdateOverCellularPermission";
const char kSetUpdateOverCellularTarget[] =
    "SetUpdateOverCellularTarget";

// Signals.
const char kStatusUpdate[] = "StatusUpdate";

// Flags used in the AttemptUpdateWithFlags() D-Bus method.
typedef enum {
  kAttemptUpdateFlagNonInteractive = (1 << 0)
} AttemptUpdateFlags;

// End of Life status used in the GetEolStatus() D-Bus method.
typedef enum {
   kSupported = 0,
   kSecurityOnly = 1,
   kEol = 2,
} EndOfLifeStatus;

// Operations contained in StatusUpdate signals.
const char kUpdateStatusIdle[] = "UPDATE_STATUS_IDLE";
const char kUpdateStatusCheckingForUpdate[] =
    "UPDATE_STATUS_CHECKING_FOR_UPDATE";
const char kUpdateStatusUpdateAvailable[] = "UPDATE_STATUS_UPDATE_AVAILABLE";
const char kUpdateStatusDownloading[] = "UPDATE_STATUS_DOWNLOADING";
const char kUpdateStatusVerifying[] = "UPDATE_STATUS_VERIFYING";
const char kUpdateStatusFinalizing[] = "UPDATE_STATUS_FINALIZING";
const char kUpdateStatusUpdatedNeedReboot[] =
    "UPDATE_STATUS_UPDATED_NEED_REBOOT";
const char kUpdateStatusReportingErrorEvent[] =
    "UPDATE_STATUS_REPORTING_ERROR_EVENT";
const char kUpdateStatusAttemptingRollback[] =
    "UPDATE_STATUS_ATTEMPTING_ROLLBACK";
const char kUpdateStatusDisabled[] = "UPDATE_STATUS_DISABLED";
const char kUpdateStatusNeedPermissionToUpdate[] =
    "UPDATE_STATUS_NEED_PERMISSION_TO_UPDATE";
}  // namespace update_engine

#endif  // SYSTEM_API_DBUS_UPDATE_ENGINE_DBUS_CONSTANTS_H_

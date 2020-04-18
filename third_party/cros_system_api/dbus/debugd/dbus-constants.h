// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_DEBUGD_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_DEBUGD_DBUS_CONSTANTS_H_

namespace debugd {
const char kDebugdInterface[] = "org.chromium.debugd";
const char kDebugdServicePath[] = "/org/chromium/debugd";
const char kDebugdServiceName[] = "org.chromium.debugd";

// Methods.
const char kCupsAddManuallyConfiguredPrinter[] =
    "CupsAddManuallyConfiguredPrinter";
const char kCupsAddAutoConfiguredPrinter[] = "CupsAddAutoConfiguredPrinter";
const char kCupsRemovePrinter[] = "CupsRemovePrinter";
const char kDumpDebugLogs[] = "DumpDebugLogs";
const char kGetInterfaces[] = "GetInterfaces";
const char kGetModemStatus[] = "GetModemStatus";
const char kGetNetworkStatus[] = "GetNetworkStatus";
const char kGetPerfOutput[] = "GetPerfOutput";
const char kGetPerfOutputFd[] = "GetPerfOutputFd";
const char kGetRoutes[] = "GetRoutes";
const char kGetWiMaxStatus[] = "GetWiMaxStatus";
const char kSetDebugMode[] = "SetDebugMode";
const char kSystraceStart[] = "SystraceStart";
const char kSystraceStop[] = "SystraceStop";
const char kSystraceStatus[] = "SystraceStatus";
const char kGetLog[] = "GetLog";
const char kGetAllLogs[] = "GetAllLogs";
const char kGetUserLogFiles[] = "GetUserLogFiles";
const char kGetFeedbackLogs[] = "GetFeedbackLogs";
const char kGetBigFeedbackLogs[] = "GetBigFeedbackLogs";
const char kTestICMP[] = "TestICMP";
const char kTestICMPWithOptions[] = "TestICMPWithOptions";
const char kLogKernelTaskStates[] = "LogKernelTaskStates";
const char kUploadCrashes[] = "UploadCrashes";
const char kRemoveRootfsVerification[] = "RemoveRootfsVerification";
const char kEnableChromeRemoteDebugging[] = "EnableChromeRemoteDebugging";
const char kEnableBootFromUsb[] = "EnableBootFromUsb";
const char kConfigureSshServer[] = "ConfigureSshServer";
const char kSetUserPassword[] = "SetUserPassword";
const char kEnableChromeDevFeatures[] = "EnableChromeDevFeatures";
const char kQueryDevFeatures[] = "QueryDevFeatures";
const char kSetOomScoreAdj[] = "SetOomScoreAdj";
const char kStartVmConcierge[] = "StartVmConcierge";
const char kStopVmConcierge[] = "StopVmConcierge";
const char kSetRlzPingSent[] = "SetRlzPingSent";

// Values.
enum DevFeatureFlag {
  DEV_FEATURES_DISABLED = 1 << 0,
  DEV_FEATURE_ROOTFS_VERIFICATION_REMOVED = 1 << 1,
  DEV_FEATURE_BOOT_FROM_USB_ENABLED = 1 << 2,
  DEV_FEATURE_SSH_SERVER_CONFIGURED = 1 << 3,
  DEV_FEATURE_DEV_MODE_ROOT_PASSWORD_SET = 1 << 4,
  DEV_FEATURE_SYSTEM_ROOT_PASSWORD_SET = 1 << 5,
  DEV_FEATURE_CHROME_REMOTE_DEBUGGING_ENABLED = 1 << 6,
};

// CupsAdd* error codes
enum CupsResult {
  CUPS_SUCCESS = 0,
  CUPS_FATAL = 1,
  CUPS_INVALID_PPD = 2,
  CUPS_LPADMIN_FAILURE = 3,
  CUPS_AUTOCONF_FAILURE = 4,
};

// Debug log keys which should be substituted in the system info dialog.
const char kIwlwifiDumpKey[] = "iwlwifi_dump";

}  // namespace debugd

#endif  // SYSTEM_API_DBUS_DEBUGD_DBUS_CONSTANTS_H_

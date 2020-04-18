// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_CHROME_CLEANER_CONSTANTS_CHROME_CLEANER_SWITCHES_H_
#define CHROME_CHROME_CLEANER_CONSTANTS_CHROME_CLEANER_SWITCHES_H_

namespace chrome_cleaner {

// Command line switches.
extern const char kCleaningTimeoutMinutesSwitch[];
extern const char kCleanupIdSwitch[];
extern const char kCrashHandlerSwitch[];
extern const char kCrashSwitch[];
extern const char kDumpRawLogsSwitch[];
extern const char kElevatedSwitch[];
extern const char kForceLogsUploadFailureSwitch[];
extern const char kForceRecoveryComponentSwitch[];
extern const char kForceSelfDeleteSwitch[];
extern const char kForceUwsDetectionSwitch[];
extern const char kInitDoneNotifierSwitch[];
extern const char kIntegrationTestTimeoutMinutesSwitch[];
extern const char kLoadEmptyDLLSwitch[];
extern const char kLogInterfaceCallsToSwitch[];
extern const char kLogUploadRetryIntervalSwitch[];
extern const char kNoCrashUploadSwitch[];
extern const char kNoRecoveryComponentSwitch[];
extern const char kNoReportUploadSwitch[];
extern const char kNoSelfDeleteSwitch[];
extern const char kPostRebootSwitch[];
extern const char kPostRebootSwitchesInOtherRegistryKeySwitch[];
extern const char kPostRebootTriggerSwitch[];
extern const char kRemoveScanOnlyUwS[];
extern const char kSandboxMojoPipeTokenSwitch[];
extern const char kSandboxedProcessIdSwitch[];
extern const char kScanningTimeoutMinutesSwitch[];
extern const char kTestLoggingURLSwitch[];
extern const char kTestingSwitch[];
extern const char kUploadLogFileSwitch[];
extern const char kUseCrashHandlerInTestsSwitch[];
extern const char kUseCrashHandlerWithIdSwitch[];
extern const char kUseTempRegistryPathSwitch[];
extern const char kUserResponseTimeoutMinutesSwitch[];
extern const char kWithCleanupModeLogsSwitch[];

// Unoffical build only switches.
#if !defined(CHROME_CLEANER_OFFICIAL_BUILD)
extern const char kAllowUnsecureDLLsSwitch[];
#endif  // CHROME_CLEANER_OFFICIAL_BUILD

}  // namespace chrome_cleaner

#endif  // CHROME_CHROME_CLEANER_CONSTANTS_CHROME_CLEANER_SWITCHES_H_

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_NOTIFICATION_HELPER_NOTIFICATION_HELPER_CRASH_REPORTER_CLIENT_H_
#define CHROME_NOTIFICATION_HELPER_NOTIFICATION_HELPER_CRASH_REPORTER_CLIENT_H_

#include "base/files/file_path.h"
#include "base/macros.h"
#include "components/crash/content/app/crash_reporter_client.h"

class NotificationHelperCrashReporterClient
    : public crash_reporter::CrashReporterClient {
 public:
  // Instantiates a process wide instance of the
  // NotificationHelperCrashReporterClient class and initializes crash reporting
  // for the process. The instance is leaked.
  // Uses the crashpad handler embedded in the executable at |exe_path|.
  static void InitializeCrashReportingForProcessWithHandler(
      const base::FilePath& exe_path);

  NotificationHelperCrashReporterClient();
  ~NotificationHelperCrashReporterClient() override;

  // crash_reporter::CrashReporterClient:
  bool ShouldCreatePipeName(const base::string16& process_type) override;
  bool GetAlternativeCrashDumpLocation(base::string16* crash_dir) override;
  void GetProductNameAndVersion(const base::string16& exe_path,
                                base::string16* product_name,
                                base::string16* version,
                                base::string16* special_build,
                                base::string16* channel_name) override;
  bool ShouldShowRestartDialog(base::string16* title,
                               base::string16* message,
                               bool* is_rtl_locale) override;
  bool AboutToRestart() override;
  bool GetDeferredUploadsSupported(bool is_per_user_install) override;
  bool GetIsPerUserInstall() override;
  bool GetShouldDumpLargerDumps() override;
  int GetResultCodeRespawnFailed() override;
  bool GetCrashDumpLocation(base::string16* crash_dir) override;
  bool GetCrashMetricsLocation(base::string16* metrics_dir) override;
  bool IsRunningUnattended() override;
  bool GetCollectStatsConsent() override;
  bool GetCollectStatsInSample() override;
  bool ReportingIsEnforcedByPolicy(bool* enabled) override;
  bool ShouldMonitorCrashHandlerExpensively() override;
  bool EnableBreakpadForProcess(const std::string& process_type) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationHelperCrashReporterClient);
};

#endif  // CHROME_NOTIFICATION_HELPER_NOTIFICATION_HELPER_CRASH_REPORTER_CLIENT_H_

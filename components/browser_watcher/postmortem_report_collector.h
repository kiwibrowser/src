// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Following an unclean shutdown, a stability report can be collected and
// submitted for upload to a reporter.

#ifndef COMPONENTS_BROWSER_WATCHER_POSTMORTEM_REPORT_COLLECTOR_H_
#define COMPONENTS_BROWSER_WATCHER_POSTMORTEM_REPORT_COLLECTOR_H_

#include <stdio.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/debug/activity_analyzer.h"
#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/browser_watcher/stability_report.pb.h"
#include "components/browser_watcher/stability_report_extractor.h"
#include "components/metrics/system_session_analyzer_win.h"
#include "third_party/crashpad/crashpad/client/crash_report_database.h"
#include "third_party/crashpad/crashpad/util/file/file_writer.h"

namespace browser_watcher {

// Performs postmortem stability data collection and analysis. The data is then
// reported as user metrics (e.g. to estimate the number of unclean shutdowns,
// or those attributable to the system) and, optionally, as crash reports for
// a more detailed view.
class PostmortemReportCollector {
 public:
  // Creates a postmortem report collector. The |product_name|, |version_number|
  // and |channel_name| are used to set reporter information in postmortem
  // crash reports. If |report_database| is set, postmortem crash reports are
  // generated and registered against it. If |analyzer| is set, it used to
  // analyze the containing system session.
  PostmortemReportCollector(metrics::SystemSessionAnalyzer* analyzer);
  PostmortemReportCollector(const std::string& product_name,
                            const std::string& version_number,
                            const std::string& channel_name,
                            crashpad::CrashReportDatabase* report_database,
                            metrics::SystemSessionAnalyzer* analyzer);
  ~PostmortemReportCollector();

  // Analyzes |stability_files|, logs postmortem user metrics and optionally
  // generates postmortem crash reports.
  void Process(const std::vector<base::FilePath>& stability_files);

  const std::string& product_name() const { return product_name_; }
  const std::string& version_number() const { return version_number_; }
  const std::string& channel_name() const { return channel_name_; }

 private:
  FRIEND_TEST_ALL_PREFIXES(PostmortemReportCollectorTest,
                           GetDebugStateFilePaths);
  FRIEND_TEST_ALL_PREFIXES(PostmortemReportCollectorTest, CollectEmptyFile);
  FRIEND_TEST_ALL_PREFIXES(PostmortemReportCollectorTest, CollectRandomFile);
  FRIEND_TEST_ALL_PREFIXES(PostmortemReportCollectorCollectionTest,
                           CollectSuccess);
  FRIEND_TEST_ALL_PREFIXES(
      PostmortemReportCollectorCollectionFromGlobalTrackerTest,
      LogCollection);
  FRIEND_TEST_ALL_PREFIXES(
      PostmortemReportCollectorCollectionFromGlobalTrackerTest,
      ProcessUserDataCollection);
  FRIEND_TEST_ALL_PREFIXES(
      PostmortemReportCollectorCollectionFromGlobalTrackerTest,
      FieldTrialCollection);
  FRIEND_TEST_ALL_PREFIXES(
      PostmortemReportCollectorCollectionFromGlobalTrackerTest,
      ModuleCollection);
  FRIEND_TEST_ALL_PREFIXES(
      PostmortemReportCollectorCollectionFromGlobalTrackerTest,
      SystemStateTest);

  // Processes a stability file, reports user metrics and optionally generates a
  // crash report.
  void ProcessOneReport(const crashpad::UUID& client_id,
                        const base::FilePath& file);

  virtual CollectionStatus CollectOneReport(
      const base::FilePath& stability_file,
      StabilityReport* report);

  void SetReporterDetails(StabilityReport* report) const;

  void RecordSystemShutdownState(StabilityReport* report) const;

  void GenerateCrashReport(const crashpad::UUID& client_id,
                           StabilityReport* report_proto);

  virtual bool WriteReportToMinidump(
      StabilityReport* report,
      const crashpad::UUID& client_id,
      const crashpad::UUID& report_id,
      crashpad::FileWriterInterface* minidump_file);

  std::string product_name_;
  std::string version_number_;
  std::string channel_name_;

  crashpad::CrashReportDatabase* report_database_;  // Not owned.
  metrics::SystemSessionAnalyzer* system_session_analyzer_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(PostmortemReportCollector);
};

}  // namespace browser_watcher

#endif  // COMPONENTS_BROWSER_WATCHER_POSTMORTEM_REPORT_COLLECTOR_H_

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DATA_USAGE_EXTERNAL_DATA_USE_REPORTER_H_
#define CHROME_BROWSER_ANDROID_DATA_USAGE_EXTERNAL_DATA_USE_REPORTER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "chrome/browser/android/data_usage/data_use_tab_model.h"
#include "net/base/network_change_notifier.h"

#if defined(OS_ANDROID)
#include "base/android/application_status_listener.h"
#endif

namespace data_usage {
struct DataUse;
}

namespace android {

// This class receives data use observations from ExternalDataUseObserver,
// labels the data use using DataUseTabModel, and buffers the data use report.
// The buffered reports are submitted to the platform when a minimum number of
// data usage bytes is reached, or when Chromium goes into background, or when
// the previous report submission times out. This class is not thread safe, and
// must only be accessed on UI thread.
class ExternalDataUseReporter {
 public:
  typedef base::Callback<
      bool(SessionID, const base::TimeTicks, DataUseTabModel::TrackingInfo*)>
      GetTrackingInfoCallback;

  typedef base::Callback<void(const std::string&,
                              const std::string&,
                              net::NetworkChangeNotifier::ConnectionType,
                              const std::string&,
                              const base::Time,
                              const base::Time,
                              int64_t,
                              int64_t)>
      ReportDataUseCallback;

  // Result of data usage report submission.  This enum must remain synchronized
  // with the enum of the same name in metrics/histograms/histograms.xml.
  enum DataUsageReportSubmissionResult {
    // Submission of data use report to the external observer was successful.
    DATAUSAGE_REPORT_SUBMISSION_SUCCESSFUL = 0,
    // Submission of data use report to the external observer returned error.
    DATAUSAGE_REPORT_SUBMISSION_FAILED = 1,
    // Submission of data use report to the external observer timed out.
    DATAUSAGE_REPORT_SUBMISSION_TIMED_OUT = 2,
    // Data use report was lost before an attempt was made to submit it.
    DATAUSAGE_REPORT_SUBMISSION_LOST = 3,
    DATAUSAGE_REPORT_SUBMISSION_MAX = 4
  };

  // The caller should guarantee that |data_use_tab_model| and
  // |external_data_use_observer_bridge| to be non-null during the lifetime of
  // |this|. |field_trial| is the field trial name to get the various
  // paramenters from.
  ExternalDataUseReporter(
      const char* field_trial,
      const GetTrackingInfoCallback& get_tracking_info_callback,
      const ReportDataUseCallback& report_data_use_callback);

  virtual ~ExternalDataUseReporter();

  void InitOnUIThread();

  // Notifies the ExternalDataUseReporter of data usage. The data use is labeled
  // using |data_use_tab_model_|, buffered and then reported to
  // |external_data_use_observer_bridge_| later.
  void OnDataUse(std::unique_ptr<const std::vector<const data_usage::DataUse>>
                     data_use_list);

  void OnReportDataUseDone(bool success);

 private:
  friend class ExternalDataUseReporterTest;
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseReporterTest, BufferDataUseReports);
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseReporterTest, BufferSize);
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseReporterTest, DataUseReportTimedOut);
#if defined(OS_ANDROID)
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseReporterTest,
                           DataUseReportingOnApplicationStatusChange);
#endif
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseReporterTest, HashFunction);
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseReporterTest,
                           MatchingRuleFetchOnControlAppInstall);
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseReporterTest, ReportsMergedCorrectly);
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseReporterTest,
                           TimestampsMergedCorrectly);
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseReporterTest, MultipleMatchingRules);
  FRIEND_TEST_ALL_PREFIXES(ExternalDataUseReporterTest, Variations);

  // DataUseReportKey is a unique identifier for a data use report.
  struct DataUseReportKey {
    DataUseReportKey(const std::string& label,
                     const std::string& tag,
                     net::NetworkChangeNotifier::ConnectionType connection_type,
                     const std::string& mcc_mnc);

    DataUseReportKey(const DataUseReportKey& other);

    bool operator==(const DataUseReportKey& other) const;

    // Label provided by the matching rules.
    const std::string label;

    // Tag to report for the data usage.
    const std::string tag;

    // Type of network used by the request.
    const net::NetworkChangeNotifier::ConnectionType connection_type;

    // mcc_mnc operator of the provider of the SIM as obtained from
    // TelephonyManager#getNetworkOperator() Java API in Android.
    const std::string mcc_mnc;
  };

  // DataUseReport is paired with a  DataUseReportKey object. DataUseReport
  // contains the bytes send/received during a specific interval. Only the bytes
  // from the data use reports that have the |label|, |connection_type|, and
  // |mcc_mnc| specified in the corresponding DataUseReportKey object are
  // counted in the DataUseReport.
  struct DataUseReport {
    // |start_time| and |end_time| are the start and end timestamps (in UTC
    // since the standard Java epoch of 1970-01-01 00:00:00) of the interval
    // that this data report covers. |bytes_downloaded| and |bytes_uploaded| are
    // the total bytes received and send during this interval.
    DataUseReport(const base::Time& start_time,
                  const base::Time& end_time,
                  int64_t bytes_downloaded,
                  int64_t bytes_uploaded);

    // Start time of |this| data report (in UTC since the standard Java epoch of
    // 1970-01-01 00:00:00).
    const base::Time start_time;

    // End time of |this| data report (in UTC since the standard Java epoch of
    // 1970-01-01 00:00:00)
    const base::Time end_time;

    // Number of bytes downloaded and uploaded by Chromium from |start_time| to
    // |end_time|.
    const int64_t bytes_downloaded;
    const int64_t bytes_uploaded;
  };

  // Class that implements hash operator on DataUseReportKey.
  class DataUseReportKeyHash {
   public:
    // A simple heuristical hash function that satisifes the property that two
    // equal data structures have the same hash value.
    size_t operator()(const DataUseReportKey& k) const;
  };

  typedef std::
      unordered_map<DataUseReportKey, DataUseReport, DataUseReportKeyHash>
          DataUseReports;

  // Maximum size of the data use report buffer. Once this limit is reached, new
  // reports will be ignored.
  // TODO(rajendrant): Instead of the ignoring the new data use report, remove
  // the oldest report entry.
  static const size_t kMaxBufferSize;

  // Adds |data_use| to buffered reports. |data_use| is the received data use
  // report. |label| is a non-empty label that applies to |data_use|. |tag| is
  // the tag to be applied for this data use. |start_time| and |end_time| are
  // the start, and end times of the interval during which bytes reported in
  // |data_use| went over the network.
  void BufferDataUseReport(const data_usage::DataUse& data_use,
                           const std::string& label,
                           const std::string& tag,
                           const base::Time& start_time,
                           const base::Time& end_time);

  // Submits the first data report among the buffered data reports in
  // |buffered_data_reports_|. Since an unordered map is used to buffer the
  // reports, the order of reports may change. The reports are buffered in an
  // arbitrary order and there are no guarantees that the next report to be
  // submitted is the oldest one buffered. |immediate| indicates whether to
  // submit the report immediately or to wait until |data_use_report_min_bytes_|
  // unreported bytes are buffered.
  void SubmitBufferedDataUseReport(bool immediate);

#if defined(OS_ANDROID)
  // Called whenever the application transitions from foreground to background
  // or vice versa.
  void OnApplicationStateChange(base::android::ApplicationState new_state);
#endif

  // Callback to be run to get the tracking info for a tab at a particular time.
  const GetTrackingInfoCallback get_tracking_info_callback_;

  // Callback to be run to report the data usage to the underlying platform.
  const ReportDataUseCallback report_data_use_callback_;

#if defined(OS_ANDROID)
  // Listens to when Chromium gets backgrounded and submits buffered data use
  // reports.
  std::unique_ptr<base::android::ApplicationStatusListener> app_state_listener_;
#endif

  // Time when the currently pending data use report was submitted.
  // |last_data_report_submitted_ticks_| is null if no data use report is
  // currently pending.
  base::TimeTicks last_data_report_submitted_ticks_;

  // |pending_report_bytes_| is the total byte count in the data use report that
  // is currently pending.
  int64_t pending_report_bytes_;

  // Time when the data use reports were last received.
  base::Time previous_report_time_;

  // Total number of bytes transmitted or received across all the buffered
  // reports.
  int64_t total_bytes_buffered_;

  // Buffered data reports that need to be submitted to the
  // |external_data_use_observer_bridge_|.
  DataUseReports buffered_data_reports_;

  // Minimum number of bytes that should be buffered before a data use report is
  // submitted.
  const int64_t data_use_report_min_bytes_;

  // If a data use report is pending for more than |data_report_submit_timeout_|
  // duration, it is considered as timed out.
  const base::TimeDelta data_report_submit_timeout_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(ExternalDataUseReporter);
};

}  // namespace android

#endif  // CHROME_BROWSER_ANDROID_DATA_USAGE_EXTERNAL_DATA_USE_REPORTER_H_

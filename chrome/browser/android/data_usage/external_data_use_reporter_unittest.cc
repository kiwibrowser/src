// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/data_usage/external_data_use_reporter.h"

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/metrics/field_trial.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/android/data_usage/data_use_tab_model.h"
#include "chrome/browser/android/data_usage/external_data_use_observer.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/data_usage/core/data_use.h"
#include "components/data_usage/core/data_use_aggregator.h"
#include "components/sessions/core/session_id.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

const char kUMAMatchingRuleFirstFetchDurationHistogram[] =
    "DataUsage.Perf.MatchingRuleFirstFetchDuration";
const char kUMAReportSubmissionDurationHistogram[] =
    "DataUsage.Perf.ReportSubmissionDuration";

const char kDefaultLabel[] = "label";
const SessionID kDefaultTabId = SessionID::FromSerializedValue(1);
const char kDefaultURL[] = "http://www.google.com/#q=abc";

const char kFooMccMnc[] = "foo_mccmnc";
const char kBarMccMnc[] = "bar_mccmnc";
const char kBazMccMnc[] = "baz_mccmnc";
const char kFooLabel[] = "foo_label";
const char kBarLabel[] = "bar_label";

}  // namespace

namespace android {

class ExternalDataUseReporterTest : public testing::Test {
 public:
  void SetUp() override {
    thread_bundle_.reset(new content::TestBrowserThreadBundle(
        content::TestBrowserThreadBundle::IO_MAINLOOP));
    profile_manager_.reset(
        new TestingProfileManager(TestingBrowserProcess::GetGlobal()));
    EXPECT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile("p1");

    io_task_runner_ = content::BrowserThread::GetTaskRunnerForThread(
        content::BrowserThread::IO);
    ui_task_runner_ = content::BrowserThread::GetTaskRunnerForThread(
        content::BrowserThread::UI);
    data_use_aggregator_.reset(
        new data_usage::DataUseAggregator(nullptr, nullptr));
    external_data_use_observer_.reset(new ExternalDataUseObserver(
        data_use_aggregator_.get(), io_task_runner_.get(),
        ui_task_runner_.get()));
    // Wait for |external_data_use_observer_| to create the Java object.
    base::RunLoop().RunUntilIdle();
    data_use_tab_model()->is_control_app_installed_ = true;
  }

  // Replaces |external_data_use_observer_| with a new ExternalDataUseObserver.
  void ReplaceExternalDataUseObserver(
      std::map<std::string, std::string> variation_params) {
    variations::testing::ClearAllVariationParams();
    ASSERT_TRUE(variations::AssociateVariationParams(
        ExternalDataUseObserver::kExternalDataUseObserverFieldTrial, "Enabled",
        variation_params));

    base::FieldTrialList field_trial_list(nullptr);

    base::FieldTrialList::CreateFieldTrial(
        ExternalDataUseObserver::kExternalDataUseObserverFieldTrial, "Enabled");

    external_data_use_observer_.reset(new ExternalDataUseObserver(
        data_use_aggregator_.get(), io_task_runner_.get(),
        ui_task_runner_.get()));
    // Wait for |external_data_use_observer_| to create the Java object.
    base::RunLoop().RunUntilIdle();
  }

  void FetchMatchingRulesDone(const std::vector<std::string>& app_package_name,
                              const std::vector<std::string>& domain_path_regex,
                              const std::vector<std::string>& label) {
    data_use_tab_model()->RegisterURLRegexes(app_package_name,
                                             domain_path_regex, label);
    base::RunLoop().RunUntilIdle();
  }

  // Adds a default matching rule to |external_data_use_observer_|.
  void AddDefaultMatchingRule() {
    std::vector<std::string> url_regexes;
    url_regexes.push_back(
        "http://www[.]google[.]com/#q=.*|https://www[.]google[.]com/#q=.*");
    FetchMatchingRulesDone(
        std::vector<std::string>(url_regexes.size(), std::string()),
        url_regexes,
        std::vector<std::string>(url_regexes.size(), kDefaultLabel));
  }

  // Notifies DataUseTabModel that tab tracking has started on kDefaultTabId.
  void TriggerTabTrackingOnDefaultTab() {
    data_use_tab_model()->OnNavigationEvent(
        kDefaultTabId, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH,
        GURL(kDefaultURL), std::string(), nullptr);
  }

  // Returns a default data_usage::DataUse object.
  data_usage::DataUse default_data_use() {
    return data_usage::DataUse(
        GURL(kDefaultURL), base::TimeTicks::Now(), GURL(), kDefaultTabId,
        net::NetworkChangeNotifier::CONNECTION_UNKNOWN, "",
        default_upload_bytes(), default_download_bytes());
  }

  void OnDataUse(const data_usage::DataUse& data_use) {
    std::unique_ptr<std::vector<const data_usage::DataUse>> data_use_list(
        new std::vector<const data_usage::DataUse>());
    data_use_list->push_back(data_use);
    external_data_use_reporter()->OnDataUse(std::move(data_use_list));
  }

  ExternalDataUseObserver* external_data_use_observer() const {
    return external_data_use_observer_.get();
  }

  ExternalDataUseReporter* external_data_use_reporter() const {
    return external_data_use_observer_->GetExternalDataUseReporterForTesting();
  }

  DataUseTabModel* data_use_tab_model() const {
    return external_data_use_observer()->GetDataUseTabModel();
  }

  const ExternalDataUseReporter::DataUseReports& buffered_data_reports() const {
    return external_data_use_reporter()->buffered_data_reports_;
  }

  int64_t default_upload_bytes() const { return 1; }

  int64_t default_download_bytes() const {
    return external_data_use_reporter()->data_use_report_min_bytes_;
  }

 private:
  std::unique_ptr<content::TestBrowserThreadBundle> thread_bundle_;
  std::unique_ptr<data_usage::DataUseAggregator> data_use_aggregator_;
  std::unique_ptr<ExternalDataUseObserver> external_data_use_observer_;

  std::unique_ptr<TestingProfileManager> profile_manager_;

  // Test profile used by the tests is owned by |profile_manager_|.
  TestingProfile* profile_;

  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;
};

// Verifies that buffer size does not exceed the specified limit.
TEST_F(ExternalDataUseReporterTest, BufferSize) {
  base::HistogramTester histogram_tester;

  AddDefaultMatchingRule();
  TriggerTabTrackingOnDefaultTab();

  // Push more entries than the buffer size. Buffer size should not be exceeded.
  for (size_t i = 0; i < ExternalDataUseReporter::kMaxBufferSize * 2; ++i) {
    data_usage::DataUse data_use = default_data_use();
    data_use.mcc_mnc = "mccmnc" + base::Int64ToString(i);
    OnDataUse(data_use);
  }

  EXPECT_LE(0, external_data_use_reporter()->total_bytes_buffered_);

  // Verify that total buffered bytes is computed correctly.
  EXPECT_EQ(
      static_cast<int64_t>(ExternalDataUseReporter::kMaxBufferSize *
                           (default_upload_bytes() + default_download_bytes())),
      external_data_use_reporter()->total_bytes_buffered_);
  EXPECT_EQ(ExternalDataUseReporter::kMaxBufferSize,
            buffered_data_reports().size());

  // Verify the label of the data use report.
  for (const auto& it : buffered_data_reports())
    EXPECT_EQ(kDefaultLabel, it.first.label);

  // Verify that metrics were updated correctly for the lost reports.
  histogram_tester.ExpectUniqueSample(
      "DataUsage.ReportSubmissionResult",
      ExternalDataUseReporter::DATAUSAGE_REPORT_SUBMISSION_LOST,
      ExternalDataUseReporter::kMaxBufferSize - 1);
  histogram_tester.ExpectUniqueSample(
      "DataUsage.ReportSubmission.Bytes.Lost",
      default_upload_bytes() + default_download_bytes(),
      ExternalDataUseReporter::kMaxBufferSize - 1);
}

// Tests that buffered data use reports are merged correctly.
TEST_F(ExternalDataUseReporterTest, ReportsMergedCorrectly) {
  AddDefaultMatchingRule();
  TriggerTabTrackingOnDefaultTab();

  const size_t num_iterations = ExternalDataUseReporter::kMaxBufferSize * 2;

  for (size_t i = 0; i < num_iterations; ++i) {
    data_usage::DataUse data_use_foo = default_data_use();
    data_use_foo.mcc_mnc = kFooMccMnc;
    OnDataUse(data_use_foo);

    data_usage::DataUse data_use_bar = default_data_use();
    data_use_bar.mcc_mnc = kBarMccMnc;
    OnDataUse(data_use_bar);

    data_usage::DataUse data_use_baz = default_data_use();
    data_use_baz.mcc_mnc = kBazMccMnc;
    OnDataUse(data_use_baz);
  }

  ASSERT_EQ(3U, buffered_data_reports().size());

  // One of the foo reports should have been submitted, and all the other foo
  // reports should have been merged together. All of the bar and baz reports
  // should have been merged together respectively.
  const struct {
    std::string mcc_mnc;
    size_t number_of_merged_reports;
  } expected_data_use_reports[] = {{kFooMccMnc, num_iterations - 1},
                                   {kBarMccMnc, num_iterations},
                                   {kBazMccMnc, num_iterations}};

  for (const auto& expected_report : expected_data_use_reports) {
    const ExternalDataUseReporter::DataUseReportKey key(
        kDefaultLabel, DataUseTabModel::kDefaultTag,
        net::NetworkChangeNotifier::CONNECTION_UNKNOWN,
        expected_report.mcc_mnc);

    EXPECT_NE(buffered_data_reports().end(), buffered_data_reports().find(key));
    EXPECT_EQ(static_cast<int64_t>(expected_report.number_of_merged_reports) *
                  (default_download_bytes()),
              buffered_data_reports().find(key)->second.bytes_downloaded);
    EXPECT_EQ(static_cast<int64_t>(expected_report.number_of_merged_reports *
                                   (default_upload_bytes())),
              buffered_data_reports().find(key)->second.bytes_uploaded);
  }
}

// Tests that timestamps of merged reports is correct.
TEST_F(ExternalDataUseReporterTest, TimestampsMergedCorrectly) {
  AddDefaultMatchingRule();

  const size_t num_iterations = ExternalDataUseReporter::kMaxBufferSize * 2;

  base::Time start_timestamp = base::Time::UnixEpoch();
  base::Time end_timestamp = start_timestamp + base::TimeDelta::FromSeconds(1);
  for (size_t i = 0; i < num_iterations; ++i) {
    external_data_use_reporter()->BufferDataUseReport(
        default_data_use(), kDefaultLabel, DataUseTabModel::kDefaultTag,
        start_timestamp, end_timestamp);

    start_timestamp += base::TimeDelta::FromSeconds(1);
    end_timestamp += base::TimeDelta::FromSeconds(1);
  }

  EXPECT_EQ(1U, buffered_data_reports().size());
  EXPECT_EQ(0, buffered_data_reports().begin()->second.start_time.ToJavaTime());
  // Convert from seconds to milliseconds.
  EXPECT_EQ(static_cast<int64_t>(num_iterations * 1000),
            buffered_data_reports().begin()->second.end_time.ToJavaTime());
}

// Tests the behavior when multiple matching rules are available for URL and
// package name matching.
TEST_F(ExternalDataUseReporterTest, MultipleMatchingRules) {
  std::vector<std::string> url_regexes;
  url_regexes.push_back(
      "http://www[.]foo[.]com/#q=.*|https://www[.]foo[.]com/#q=.*");
  url_regexes.push_back(
      "http://www[.]bar[.]com/#q=.*|https://www[.]bar[.]com/#q=.*");

  std::vector<std::string> labels;
  labels.push_back(kFooLabel);
  labels.push_back(kBarLabel);

  std::vector<std::string> app_package_names;
  const char kAppFoo[] = "com.example.foo";
  const char kAppBar[] = "com.example.bar";
  app_package_names.push_back(kAppFoo);
  app_package_names.push_back(kAppBar);

  FetchMatchingRulesDone(app_package_names, url_regexes, labels);

  data_use_tab_model()->OnNavigationEvent(
      kDefaultTabId, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH,
      GURL("http://www.foo.com/#q=abc"), std::string(), nullptr);

  const SessionID kTabIdBar = SessionID::FromSerializedValue(2);
  data_use_tab_model()->OnNavigationEvent(
      kTabIdBar, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH,
      GURL("http://www.bar.com/#q=abc"), std::string(), nullptr);

  EXPECT_EQ(0U, external_data_use_reporter()->buffered_data_reports_.size());
  EXPECT_TRUE(external_data_use_reporter()
                  ->last_data_report_submitted_ticks_.is_null());

  // Check |kLabelFoo| matching rule.
  data_usage::DataUse data_foo = default_data_use();
  data_foo.url = GURL("http://www.foo.com/#q=abc");
  data_foo.mcc_mnc = kFooMccMnc;
  OnDataUse(data_foo);

  // Check |kLabelBar| matching rule.
  data_usage::DataUse data_bar = default_data_use();
  data_bar.tab_id = kTabIdBar;
  data_bar.url = GURL("http://www.bar.com/#q=abc");
  data_bar.mcc_mnc = kBarMccMnc;
  OnDataUse(data_bar);

  // bar report should be present.
  EXPECT_EQ(1U, buffered_data_reports().size());

  const ExternalDataUseReporter::DataUseReportKey key_bar(
      kBarLabel, DataUseTabModel::kDefaultTag,
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN, kBarMccMnc);
  EXPECT_NE(buffered_data_reports().end(),
            buffered_data_reports().find(key_bar));
}

// Tests that hash function reports distinct values. This test may fail if there
// is a hash collision, however the chances of that happening are very low.
TEST_F(ExternalDataUseReporterTest, HashFunction) {
  ExternalDataUseReporter::DataUseReportKeyHash hash;

  ExternalDataUseReporter::DataUseReportKey foo(
      kFooLabel, DataUseTabModel::kDefaultTag,
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN, kFooMccMnc);
  ExternalDataUseReporter::DataUseReportKey bar_label(
      kBarLabel, DataUseTabModel::kDefaultTag,
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN, kFooMccMnc);
  ExternalDataUseReporter::DataUseReportKey bar_custom_tab_tag(
      kBarLabel, DataUseTabModel::kCustomTabTag,
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN, kFooMccMnc);
  ExternalDataUseReporter::DataUseReportKey bar_network_type(
      kFooLabel, DataUseTabModel::kDefaultTag,
      net::NetworkChangeNotifier::CONNECTION_WIFI, kFooMccMnc);
  ExternalDataUseReporter::DataUseReportKey bar_mcc_mnc(
      kFooLabel, DataUseTabModel::kDefaultTag,
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN, kBarMccMnc);

  EXPECT_NE(hash(foo), hash(bar_label));
  EXPECT_NE(hash(foo), hash(bar_custom_tab_tag));
  EXPECT_NE(hash(foo), hash(bar_network_type));
  EXPECT_NE(hash(foo), hash(bar_mcc_mnc));
}

// Tests if data use reports are sent only after the total bytes sent/received
// across all buffered reports have reached the specified threshold.
TEST_F(ExternalDataUseReporterTest, BufferDataUseReports) {
  AddDefaultMatchingRule();
  TriggerTabTrackingOnDefaultTab();

  // This tests reports 1024 bytes in each loop iteration. For the test to work
  // properly, |data_use_report_min_bytes_| should be a multiple of 1024.
  ASSERT_EQ(0, external_data_use_reporter()->data_use_report_min_bytes_ % 1024);

  const size_t num_iterations =
      external_data_use_reporter()->data_use_report_min_bytes_ / 1024;

  for (size_t i = 0; i < num_iterations; ++i) {
    data_usage::DataUse data_use = default_data_use();
    data_use.tx_bytes = 1024;
    data_use.rx_bytes = 0;
    OnDataUse(data_use);

    if (i != num_iterations - 1) {
      // Total buffered bytes is less than the minimum threshold. Data use
      // report should not be sent.
      EXPECT_TRUE(external_data_use_reporter()
                      ->last_data_report_submitted_ticks_.is_null());
      EXPECT_EQ(static_cast<int64_t>(i + 1),
                external_data_use_reporter()->total_bytes_buffered_ / 1024);
      EXPECT_EQ(0, external_data_use_reporter()->pending_report_bytes_);

    } else {
      // Total bytes is at least the minimum threshold. This should trigger
      // submitting of the buffered data use report.
      EXPECT_FALSE(external_data_use_reporter()
                       ->last_data_report_submitted_ticks_.is_null());
      EXPECT_EQ(0, external_data_use_reporter()->total_bytes_buffered_);
    }
  }
  EXPECT_EQ(0, external_data_use_reporter()->total_bytes_buffered_);
  EXPECT_EQ(static_cast<int64_t>(num_iterations),
            external_data_use_reporter()->pending_report_bytes_ / 1024);

  base::HistogramTester histogram_tester;
  external_data_use_reporter()->OnReportDataUseDone(true);

  // Verify that metrics were updated correctly for the report that was
  // successfully submitted.
  histogram_tester.ExpectUniqueSample(
      "DataUsage.ReportSubmissionResult",
      ExternalDataUseReporter::DATAUSAGE_REPORT_SUBMISSION_SUCCESSFUL, 1);
  histogram_tester.ExpectUniqueSample(
      "DataUsage.ReportSubmission.Bytes.Successful",
      external_data_use_reporter()->data_use_report_min_bytes_, 1);
  histogram_tester.ExpectTotalCount(kUMAReportSubmissionDurationHistogram, 1);

  // Verify that metrics were updated correctly for the report that was not
  // successfully submitted.
  OnDataUse(default_data_use());
  external_data_use_reporter()->OnReportDataUseDone(false);
  histogram_tester.ExpectTotalCount("DataUsage.ReportSubmissionResult", 2);
  histogram_tester.ExpectBucketCount(
      "DataUsage.ReportSubmissionResult",
      ExternalDataUseReporter::DATAUSAGE_REPORT_SUBMISSION_FAILED, 1);
  histogram_tester.ExpectUniqueSample(
      "DataUsage.ReportSubmission.Bytes.Failed",
      external_data_use_reporter()->data_use_report_min_bytes_, 1);
  histogram_tester.ExpectTotalCount(kUMAReportSubmissionDurationHistogram, 2);
}

#if defined(OS_ANDROID)
// Tests data use report submission when application status callback is called.
// Report should be submitted even if the number of bytes is less than the
// threshold. Report should not be submitted if there is a pending report.
TEST_F(ExternalDataUseReporterTest, DataUseReportingOnApplicationStatusChange) {
  base::HistogramTester histogram_tester;
  AddDefaultMatchingRule();
  TriggerTabTrackingOnDefaultTab();

  // Report with less than threshold bytes should be reported, on application
  // state change to background.
  data_usage::DataUse data_use = default_data_use();
  data_use.tx_bytes = 1;
  data_use.rx_bytes = 1;
  OnDataUse(data_use);
  EXPECT_TRUE(external_data_use_reporter()
                  ->last_data_report_submitted_ticks_.is_null());
  EXPECT_EQ(2, external_data_use_reporter()->total_bytes_buffered_);
  EXPECT_EQ(0, external_data_use_reporter()->pending_report_bytes_);

  external_data_use_reporter()->OnApplicationStateChange(
      base::android::APPLICATION_STATE_HAS_PAUSED_ACTIVITIES);
  EXPECT_FALSE(external_data_use_reporter()
                   ->last_data_report_submitted_ticks_.is_null());
  EXPECT_EQ(0, external_data_use_reporter()->total_bytes_buffered_);
  EXPECT_EQ(2, external_data_use_reporter()->pending_report_bytes_);
  external_data_use_reporter()->OnReportDataUseDone(true);
  histogram_tester.ExpectTotalCount(kUMAReportSubmissionDurationHistogram, 1);

  // Create pending report.
  OnDataUse(default_data_use());
  EXPECT_FALSE(external_data_use_reporter()
                   ->last_data_report_submitted_ticks_.is_null());
  EXPECT_EQ(0, external_data_use_reporter()->total_bytes_buffered_);
  EXPECT_EQ(default_upload_bytes() + default_download_bytes(),
            external_data_use_reporter()->pending_report_bytes_);

  // Application state change should not submit if there is a pending report.
  data_use.tx_bytes = 1;
  data_use.rx_bytes = 1;
  OnDataUse(data_use);
  external_data_use_reporter()->OnApplicationStateChange(
      base::android::APPLICATION_STATE_HAS_PAUSED_ACTIVITIES);
  EXPECT_FALSE(external_data_use_reporter()
                   ->last_data_report_submitted_ticks_.is_null());
  EXPECT_EQ(2, external_data_use_reporter()->total_bytes_buffered_);
  EXPECT_EQ(default_upload_bytes() + default_download_bytes(),
            external_data_use_reporter()->pending_report_bytes_);
  histogram_tester.ExpectTotalCount(kUMAReportSubmissionDurationHistogram, 1);

  // Once pending report submission done callback was received, report should be
  // submitted on next application state change.
  external_data_use_reporter()->OnReportDataUseDone(true);
  external_data_use_reporter()->OnApplicationStateChange(
      base::android::APPLICATION_STATE_HAS_PAUSED_ACTIVITIES);
  EXPECT_EQ(0, external_data_use_reporter()->total_bytes_buffered_);
  EXPECT_EQ(2, external_data_use_reporter()->pending_report_bytes_);
  histogram_tester.ExpectTotalCount(kUMAReportSubmissionDurationHistogram, 2);
}
#endif  // OS_ANDROID

// Tests if the metrics are recorded correctly.
TEST_F(ExternalDataUseReporterTest, DataUseReportTimedOut) {
  base::HistogramTester histogram_tester;
  std::map<std::string, std::string> variation_params;
  variation_params["data_report_submit_timeout_msec"] = "0";
  variation_params["data_use_report_min_bytes"] = "0";

  // Create another ExternalDataUseObserver object.
  ReplaceExternalDataUseObserver(variation_params);
  histogram_tester.ExpectTotalCount(kUMAMatchingRuleFirstFetchDurationHistogram,
                                    0);

  // Trigger the control app install, and matching rules will be fetched.
  data_use_tab_model()->OnControlAppInstallStateChange(true);
  base::RunLoop().RunUntilIdle();
  histogram_tester.ExpectTotalCount(kUMAMatchingRuleFirstFetchDurationHistogram,
                                    1);

  // Verify that matching rules are fetched on every navigation after the
  // control app is installed, since there are no valid rules yet.
  data_use_tab_model()->OnNavigationEvent(
      kDefaultTabId, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH,
      GURL(kDefaultURL), std::string(), nullptr);
  base::RunLoop().RunUntilIdle();
  histogram_tester.ExpectTotalCount(kUMAMatchingRuleFirstFetchDurationHistogram,
                                    1);

  AddDefaultMatchingRule();
  TriggerTabTrackingOnDefaultTab();
  OnDataUse(default_data_use());
  OnDataUse(default_data_use());
  // First data use report should be marked as timed out.
  histogram_tester.ExpectUniqueSample(
      "DataUsage.ReportSubmissionResult",
      ExternalDataUseReporter::DATAUSAGE_REPORT_SUBMISSION_TIMED_OUT, 1);
  histogram_tester.ExpectUniqueSample(
      "DataUsage.ReportSubmission.Bytes.TimedOut",
      default_upload_bytes() + default_download_bytes(), 1);
  histogram_tester.ExpectTotalCount(kUMAReportSubmissionDurationHistogram, 0);
}

// Tests if the parameters from the field trial are populated correctly.
TEST_F(ExternalDataUseReporterTest, Variations) {
  std::map<std::string, std::string> variation_params;

  const int kFetchMatchingRulesDurationSeconds = 10000;
  const int kDefaultMaxDataReportSubmitWaitMsec = 20000;
  const int64_t kDataUseReportMinBytes = 5000;
  variation_params["fetch_matching_rules_duration_seconds"] =
      base::Int64ToString(kFetchMatchingRulesDurationSeconds);
  variation_params["data_report_submit_timeout_msec"] =
      base::Int64ToString(kDefaultMaxDataReportSubmitWaitMsec);
  variation_params["data_use_report_min_bytes"] =
      base::Int64ToString(kDataUseReportMinBytes);

  // Create another ExternalDataUseObserver object.
  ReplaceExternalDataUseObserver(variation_params);
  EXPECT_EQ(
      base::TimeDelta::FromMilliseconds(kDefaultMaxDataReportSubmitWaitMsec),
      external_data_use_reporter()->data_report_submit_timeout_);
  EXPECT_EQ(kDataUseReportMinBytes,
            external_data_use_reporter()->data_use_report_min_bytes_);
}

}  // namespace android

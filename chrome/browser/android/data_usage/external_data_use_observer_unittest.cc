// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/data_usage/external_data_use_observer.h"

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

const char kDefaultLabel[] = "label";
const SessionID kDefaultTabId = SessionID::FromSerializedValue(1);
const char kDefaultURL[] = "http://www.google.com/#q=abc";

}  // namespace

namespace android {

class ExternalDataUseObserverTest : public testing::Test {
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
    external_data_use_observer()
        ->data_use_tab_model_->is_control_app_installed_ = true;
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
    external_data_use_observer_->GetDataUseTabModel()->RegisterURLRegexes(
        app_package_name, domain_path_regex, label);
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

  // Returns a default data_usage::DataUse object.
  data_usage::DataUse default_data_use() {
    return data_usage::DataUse(GURL(kDefaultURL), base::TimeTicks::Now(),
                               GURL(), kDefaultTabId,
                               net::NetworkChangeNotifier::CONNECTION_UNKNOWN,
                               "", 1 /* upload bytes*/, 1 /* download bytes */);
  }

  void OnDataUse(const data_usage::DataUse& data_use) {
    external_data_use_observer_->OnDataUse(data_use);
    base::RunLoop().RunUntilIdle();
  }

  ExternalDataUseObserver* external_data_use_observer() const {
    return external_data_use_observer_.get();
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

// Verifies that the external data use observer is registered as an observer
// only when at least one matching rule is present.
TEST_F(ExternalDataUseObserverTest, RegisteredAsDataUseObserver) {
  EXPECT_FALSE(external_data_use_observer()->registered_as_data_use_observer_);

  AddDefaultMatchingRule();
  EXPECT_TRUE(external_data_use_observer()->registered_as_data_use_observer_);

  // Push an empty vector. Since no matching rules are present,
  // |external_data_use_observer| should no longer be registered as a data use
  // observer.
  FetchMatchingRulesDone(std::vector<std::string>(), std::vector<std::string>(),
                         std::vector<std::string>());
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(external_data_use_observer()->registered_as_data_use_observer_);
}

// Tests if matching rules are fetched periodically.
TEST_F(ExternalDataUseObserverTest, PeriodicFetchMatchingRules) {
  AddDefaultMatchingRule();

  EXPECT_FALSE(
      external_data_use_observer()->last_matching_rules_fetch_time_.is_null());

  // Change the time when the fetching rules were fetched.
  external_data_use_observer()->last_matching_rules_fetch_time_ =
      base::TimeTicks::Now() -
      external_data_use_observer()->fetch_matching_rules_duration_;
  // Matching rules should be expired.
  EXPECT_GE(base::TimeTicks::Now() -
                external_data_use_observer()->last_matching_rules_fetch_time_,
            external_data_use_observer()->fetch_matching_rules_duration_);

  // OnDataUse should trigger fetching of matching rules.
  OnDataUse(default_data_use());

  // Matching rules should not be expired.
  EXPECT_LT(base::TimeTicks::Now() -
                external_data_use_observer()->last_matching_rules_fetch_time_,
            external_data_use_observer()->fetch_matching_rules_duration_);
}

// Tests the matching rule fetch behavior when the external control app is
// installed and not installed. Matching rules should be fetched when control
// app gets installed. If control app is installed and no valid rules are found,
// matching rules are fetched on every navigation. Rules are not fetched if
// control app is not installed  or if more than zero valid rules have been
// fetched.
TEST_F(ExternalDataUseObserverTest, MatchingRuleFetchOnControlAppInstall) {
  {
    // Matching rules not fetched on navigation if control app is not installed,
    // and navigation events will be buffered.
    external_data_use_observer()->last_matching_rules_fetch_time_ =
        base::TimeTicks();
    external_data_use_observer()
        ->data_use_tab_model_->is_control_app_installed_ = false;
    base::HistogramTester histogram_tester;
    external_data_use_observer()->data_use_tab_model_->OnNavigationEvent(
        kDefaultTabId, DataUseTabModel::TRANSITION_LINK, GURL(kDefaultURL),
        std::string(), nullptr);
    base::RunLoop().RunUntilIdle();
    histogram_tester.ExpectTotalCount("DataUsage.MatchingRulesCount.Valid", 0);
  }

  {
    // Matching rules are fetched when control app is installed.
    base::HistogramTester histogram_tester;
    external_data_use_observer()
        ->data_use_tab_model_->OnControlAppInstallStateChange(true);
    base::RunLoop().RunUntilIdle();
    histogram_tester.ExpectTotalCount("DataUsage.MatchingRulesCount.Valid", 1);
  }

  {
    // Matching rules fetched on every navigation if control app is installed
    // and zero rules are available.
    external_data_use_observer()->last_matching_rules_fetch_time_ =
        base::TimeTicks();
    base::HistogramTester histogram_tester;
    external_data_use_observer()->data_use_tab_model_->OnNavigationEvent(
        kDefaultTabId, DataUseTabModel::TRANSITION_LINK, GURL(kDefaultURL),
        std::string(), nullptr);
    base::RunLoop().RunUntilIdle();
    histogram_tester.ExpectTotalCount("DataUsage.MatchingRulesCount.Valid", 1);
  }

  {
    // Matching rules not fetched on navigation if control app is installed and
    // more than zero rules are available.
    AddDefaultMatchingRule();
    external_data_use_observer()->last_matching_rules_fetch_time_ =
        base::TimeTicks();
    EXPECT_TRUE(external_data_use_observer()
                    ->last_matching_rules_fetch_time_.is_null());
    base::HistogramTester histogram_tester;
    external_data_use_observer()->data_use_tab_model_->OnNavigationEvent(
        kDefaultTabId, DataUseTabModel::TRANSITION_LINK, GURL(kDefaultURL),
        std::string(), nullptr);
    base::RunLoop().RunUntilIdle();
    histogram_tester.ExpectTotalCount("DataUsage.MatchingRulesCount.Valid", 0);
  }
}

// Tests if the parameters from the field trial are populated correctly.
TEST_F(ExternalDataUseObserverTest, Variations) {
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
  EXPECT_EQ(base::TimeDelta::FromSeconds(kFetchMatchingRulesDurationSeconds),
            external_data_use_observer()->fetch_matching_rules_duration_);
}

}  // namespace android

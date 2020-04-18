// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_black_list.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/field_trial.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/simple_test_clock.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/previews/core/previews_black_list_delegate.h"
#include "components/previews/core/previews_black_list_item.h"
#include "components/previews/core/previews_experiments.h"
#include "components/previews/core/previews_opt_out_store.h"
#include "components/variations/variations_associated_data.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace previews {

namespace {

void RunLoadCallback(
    LoadBlackListCallback callback,
    std::unique_ptr<BlackListItemMap> black_list_item_map,
    std::unique_ptr<PreviewsBlackListItem> host_indifferent_black_list_item) {
  callback.Run(std::move(black_list_item_map),
               std::move(host_indifferent_black_list_item));
}

// Mock class to test that PreviewsBlackList notifies the delegate with correct
// events (e.g. New host blacklisted, user blacklisted, and blacklist cleared).
class TestPreviewsBlacklistDelegate : public PreviewsBlacklistDelegate {
 public:
  TestPreviewsBlacklistDelegate()
      : user_blacklisted_(false),
        blacklist_cleared_(false),
        blacklist_cleared_time_(base::Time::Now()) {}

  // PreviewsBlacklistDelegate:
  void OnNewBlacklistedHost(const std::string& host, base::Time time) override {
    blacklisted_hosts_[host] = time;
  }
  void OnUserBlacklistedStatusChange(bool blacklisted) override {
    user_blacklisted_ = blacklisted;
  }
  void OnBlacklistCleared(base::Time time) override {
    blacklist_cleared_ = true;
    blacklist_cleared_time_ = time;
  }

  // Gets the set of blacklisted hosts recorded.
  const std::unordered_map<std::string, base::Time>& blacklisted_hosts() const {
    return blacklisted_hosts_;
  }

  // Gets the state of user blacklisted status.
  bool user_blacklisted() const { return user_blacklisted_; }

  // Gets the state of blacklisted cleared status of |this| for testing.
  bool blacklist_cleared() const { return blacklist_cleared_; }

  // Gets the event time of blacklist is as cleared.
  base::Time blacklist_cleared_time() const { return blacklist_cleared_time_; }

 private:
  // The user blacklisted status of |this| blacklist_delegate.
  bool user_blacklisted_;

  // Check if the blacklist is notified as cleared on |this| blacklist_delegate.
  bool blacklist_cleared_;

  // The time when blacklist is cleared.
  base::Time blacklist_cleared_time_;

  // |this| blacklist_delegate's collection of blacklisted hosts.
  std::unordered_map<std::string, base::Time> blacklisted_hosts_;
};

class TestPreviewsOptOutStore : public PreviewsOptOutStore {
 public:
  TestPreviewsOptOutStore() : clear_blacklist_count_(0) {}
  ~TestPreviewsOptOutStore() override {}

  int clear_blacklist_count() { return clear_blacklist_count_; }

  // Set |host_indifferent_black_list_item_| to test behavior of
  // PreviewsBlackList on certain PreviewsOptOutStore states.
  void SetHostIndifferentBlacklistItem(
      std::unique_ptr<PreviewsBlackListItem> item) {
    host_indifferent_black_list_item_ = std::move(item);
  }

  // Set |black_list_item_map_| to test behavior of
  // PreviewsBlackList on certain PreviewsOptOutStore states.
  void SetBlacklistItemMap(std::unique_ptr<BlackListItemMap> item_map) {
    black_list_item_map_ = std::move(item_map);
  }

 private:
  // PreviewsOptOutStore implementation:
  void AddPreviewNavigation(bool opt_out,
                            const std::string& host_name,
                            PreviewsType type,
                            base::Time now) override {}

  void LoadBlackList(LoadBlackListCallback callback) override {
    if (!black_list_item_map_) {
      black_list_item_map_ = std::make_unique<BlackListItemMap>();
    }
    if (!host_indifferent_black_list_item_) {
      host_indifferent_black_list_item_ =
          PreviewsBlackList::CreateHostIndifferentBlackListItem();
    }
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&RunLoadCallback, callback,
                       std::move(black_list_item_map_),
                       std::move(host_indifferent_black_list_item_)));
  }

  void ClearBlackList(base::Time begin_time, base::Time end_time) override {
    ++clear_blacklist_count_;
  }

  int clear_blacklist_count_;
  std::unique_ptr<BlackListItemMap> black_list_item_map_;
  std::unique_ptr<PreviewsBlackListItem> host_indifferent_black_list_item_;
};

class PreviewsBlackListTest : public testing::Test {
 public:
  PreviewsBlackListTest() : field_trial_list_(nullptr), passed_reasons_({}) {}
  ~PreviewsBlackListTest() override {}

  void TearDown() override { variations::testing::ClearAllVariationParams(); }

  void StartTest(bool null_opt_out) {
    if (params_.size() > 0) {
      ASSERT_TRUE(variations::AssociateVariationParams("ClientSidePreviews",
                                                       "Enabled", params_));
      ASSERT_TRUE(base::FieldTrialList::CreateFieldTrial("ClientSidePreviews",
                                                         "Enabled"));
      params_.clear();
    }
    std::unique_ptr<TestPreviewsOptOutStore> opt_out_store =
        null_opt_out ? nullptr : std::make_unique<TestPreviewsOptOutStore>();
    opt_out_store_ = opt_out_store.get();
    black_list_ = std::make_unique<PreviewsBlackList>(
        std::move(opt_out_store), &test_clock_, &blacklist_delegate_);
    start_ = test_clock_.Now();

    passed_reasons_ = {};
  }

  void SetHostHistoryParam(size_t host_history) {
    params_["per_host_max_stored_history_length"] =
        base::NumberToString(host_history);
  }

  void SetHostIndifferentHistoryParam(size_t host_indifferent_history) {
    params_["host_indifferent_max_stored_history_length"] =
        base::NumberToString(host_indifferent_history);
  }

  void SetHostThresholdParam(int per_host_threshold) {
    params_["per_host_opt_out_threshold"] =
        base::IntToString(per_host_threshold);
  }

  void SetHostIndifferentThresholdParam(int host_indifferent_threshold) {
    params_["host_indifferent_opt_out_threshold"] =
        base::IntToString(host_indifferent_threshold);
  }

  void SetHostDurationParam(int duration_in_days) {
    params_["per_host_black_list_duration_in_days"] =
        base::IntToString(duration_in_days);
  }

  void SetSingleOptOutDurationParam(int single_opt_out_duration) {
    params_["single_opt_out_duration_in_seconds"] =
        base::IntToString(single_opt_out_duration);
  }

  void SetMaxHostInBlackListParam(size_t max_hosts_in_blacklist) {
    params_["max_hosts_in_blacklist"] =
        base::IntToString(max_hosts_in_blacklist);
  }

  // Adds an opt out and either clears the black list for a time either longer
  // or shorter than the single opt out duration parameter depending on
  // |short_time|.
  void RunClearingBlackListTest(const GURL& url, bool short_time) {
    const size_t host_indifferent_history = 1;
    const int single_opt_out_duration = 5;
    SetHostDurationParam(365);
    SetHostIndifferentHistoryParam(host_indifferent_history);
    SetHostIndifferentThresholdParam(host_indifferent_history + 1);
    SetSingleOptOutDurationParam(single_opt_out_duration);

    StartTest(false /* null_opt_out */);
    if (!short_time)
      test_clock_.Advance(
          base::TimeDelta::FromSeconds(single_opt_out_duration));

    black_list_->AddPreviewNavigation(url, true /* opt_out */,
                                      PreviewsType::OFFLINE);
    test_clock_.Advance(base::TimeDelta::FromSeconds(1));
    black_list_->ClearBlackList(start_, test_clock_.Now());
    base::RunLoop().RunUntilIdle();
  }

 protected:
  base::MessageLoop loop_;

  // Observer to |black_list_|.
  TestPreviewsBlacklistDelegate blacklist_delegate_;

  base::SimpleTestClock test_clock_;
  TestPreviewsOptOutStore* opt_out_store_;
  base::Time start_;
  std::map<std::string, std::string> params_;
  base::FieldTrialList field_trial_list_;

  std::unique_ptr<PreviewsBlackList> black_list_;
  std::vector<PreviewsEligibilityReason> passed_reasons_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PreviewsBlackListTest);
};

TEST_F(PreviewsBlackListTest, PerHostBlackListNoStore) {
  // Tests the black list behavior when a null OptOutStore is passed in.
  const GURL url_a("http://www.url_a.com");
  const GURL url_b("http://www.url_b.com");

  // Host indifferent blacklisting should have no effect with the following
  // params.
  const size_t host_indifferent_history = 1;
  SetHostHistoryParam(4);
  SetHostIndifferentHistoryParam(host_indifferent_history);
  SetHostThresholdParam(2);
  SetHostIndifferentThresholdParam(host_indifferent_history + 1);
  SetHostDurationParam(365);
  // Disable single opt out by setting duration to 0.
  SetSingleOptOutDurationParam(0);

  StartTest(true /* null_opt_out */);

  test_clock_.Advance(base::TimeDelta::FromSeconds(1));

  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_a, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  black_list_->AddPreviewNavigation(url_a, true, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_a, true, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));

  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_a, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  black_list_->AddPreviewNavigation(url_b, true, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_b, true, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));

  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_a, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  black_list_->AddPreviewNavigation(url_b, false, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_b, false, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_b, false, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));

  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_a, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  black_list_->ClearBlackList(start_, test_clock_.Now());

  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_a, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));
}

TEST_F(PreviewsBlackListTest, PerHostBlackListWithStore) {
  // Tests the black list behavior when a non-null OptOutStore is passed in.
  const GURL url_a1("http://www.url_a.com/a1");
  const GURL url_a2("http://www.url_a.com/a2");
  const GURL url_b("http://www.url_b.com");

  // Host indifferent blacklisting should have no effect with the following
  // params.
  const size_t host_indifferent_history = 1;
  SetHostHistoryParam(4);
  SetHostIndifferentHistoryParam(host_indifferent_history);
  SetHostThresholdParam(2);
  SetHostIndifferentThresholdParam(host_indifferent_history + 1);
  SetHostDurationParam(365);
  // Disable single opt out by setting duration to 0.
  SetSingleOptOutDurationParam(0);

  StartTest(false /* null_opt_out */);

  test_clock_.Advance(base::TimeDelta::FromSeconds(1));

  EXPECT_EQ(PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED,
            black_list_->IsLoadedAndAllowed(url_a1, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED,
            black_list_->IsLoadedAndAllowed(url_a2, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_a1, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_a2, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  black_list_->AddPreviewNavigation(url_a1, true, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_a1, true, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));

  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_a1, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_a2, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  black_list_->AddPreviewNavigation(url_b, true, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_b, true, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));

  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_a1, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_a2, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  black_list_->AddPreviewNavigation(url_b, false, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_b, false, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_b, false, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));

  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_a1, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_a2, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  EXPECT_EQ(0, opt_out_store_->clear_blacklist_count());
  black_list_->ClearBlackList(start_, base::Time::Now());
  EXPECT_EQ(1, opt_out_store_->clear_blacklist_count());

  EXPECT_EQ(PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED,
            black_list_->IsLoadedAndAllowed(url_a1, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED,
            black_list_->IsLoadedAndAllowed(url_a2, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, opt_out_store_->clear_blacklist_count());

  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_a1, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_a1, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));
}

TEST_F(PreviewsBlackListTest, HostIndifferentBlackList) {
  // Tests the black list behavior when a null OptOutStore is passed in.
  const GURL urls[] = {
      GURL("http://www.url_0.com"), GURL("http://www.url_1.com"),
      GURL("http://www.url_2.com"), GURL("http://www.url_3.com"),
  };

  // Per host blacklisting should have no effect with the following params.
  const size_t per_host_history = 1;
  const size_t host_indifferent_history = 4;
  const size_t host_indifferent_threshold = host_indifferent_history;
  SetHostHistoryParam(per_host_history);
  SetHostIndifferentHistoryParam(host_indifferent_history);
  SetHostThresholdParam(per_host_history + 1);
  SetHostIndifferentThresholdParam(host_indifferent_threshold);
  SetHostDurationParam(365);
  // Disable single opt out by setting duration to 0.
  SetSingleOptOutDurationParam(0);

  StartTest(true /* null_opt_out */);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));

  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(urls[0], PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(urls[1], PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(urls[2], PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(urls[3], PreviewsType::OFFLINE,
                                            &passed_reasons_));

  for (size_t i = 0; i < host_indifferent_threshold; i++) {
    black_list_->AddPreviewNavigation(urls[i], true, PreviewsType::OFFLINE);
    EXPECT_EQ(i != 3 ? PreviewsEligibilityReason::ALLOWED
                     : PreviewsEligibilityReason::USER_BLACKLISTED,
              black_list_->IsLoadedAndAllowed(urls[0], PreviewsType::OFFLINE,
                                              &passed_reasons_));
    test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  }

  EXPECT_EQ(PreviewsEligibilityReason::USER_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(urls[0], PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::USER_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(urls[1], PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::USER_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(urls[2], PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::USER_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(urls[3], PreviewsType::OFFLINE,
                                            &passed_reasons_));

  black_list_->AddPreviewNavigation(urls[3], false, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));

  // New non-opt-out entry will cause these to be allowed now.
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(urls[0], PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(urls[1], PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(urls[2], PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(urls[3], PreviewsType::OFFLINE,
                                            &passed_reasons_));
}

TEST_F(PreviewsBlackListTest, QueueBehavior) {
  // Tests the black list asynchronous queue behavior. Methods called while
  // loading the opt-out store are queued and should run in the order they were
  // queued.
  const GURL url("http://www.url.com");
  const GURL url2("http://www.url2.com");

  // Host indifferent blacklisting should have no effect with the following
  // params.
  const size_t host_indifferent_history = 1;
  SetHostIndifferentHistoryParam(host_indifferent_history);
  SetHostIndifferentThresholdParam(host_indifferent_history + 1);
  SetHostDurationParam(365);
  // Disable single opt out by setting duration to 0.
  SetSingleOptOutDurationParam(0);

  std::vector<bool> test_opt_out{true, false};

  for (auto opt_out : test_opt_out) {
    StartTest(false /* null_opt_out */);

    EXPECT_EQ(PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED,
              black_list_->IsLoadedAndAllowed(url, PreviewsType::OFFLINE,
                                              &passed_reasons_));
    black_list_->AddPreviewNavigation(url, opt_out, PreviewsType::OFFLINE);
    test_clock_.Advance(base::TimeDelta::FromSeconds(1));
    black_list_->AddPreviewNavigation(url, opt_out, PreviewsType::OFFLINE);
    test_clock_.Advance(base::TimeDelta::FromSeconds(1));
    EXPECT_EQ(PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED,
              black_list_->IsLoadedAndAllowed(url, PreviewsType::OFFLINE,
                                              &passed_reasons_));
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(opt_out ? PreviewsEligibilityReason::HOST_BLACKLISTED
                      : PreviewsEligibilityReason::ALLOWED,
              black_list_->IsLoadedAndAllowed(url, PreviewsType::OFFLINE,
                                              &passed_reasons_));
    black_list_->AddPreviewNavigation(url, opt_out, PreviewsType::OFFLINE);
    test_clock_.Advance(base::TimeDelta::FromSeconds(1));
    black_list_->AddPreviewNavigation(url, opt_out, PreviewsType::OFFLINE);
    test_clock_.Advance(base::TimeDelta::FromSeconds(1));
    EXPECT_EQ(0, opt_out_store_->clear_blacklist_count());
    black_list_->ClearBlackList(
        start_, test_clock_.Now() + base::TimeDelta::FromSeconds(1));
    EXPECT_EQ(1, opt_out_store_->clear_blacklist_count());
    black_list_->AddPreviewNavigation(url2, opt_out, PreviewsType::OFFLINE);
    test_clock_.Advance(base::TimeDelta::FromSeconds(1));
    black_list_->AddPreviewNavigation(url2, opt_out, PreviewsType::OFFLINE);
    test_clock_.Advance(base::TimeDelta::FromSeconds(1));
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(1, opt_out_store_->clear_blacklist_count());

    EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
              black_list_->IsLoadedAndAllowed(url, PreviewsType::OFFLINE,
                                              &passed_reasons_));
    EXPECT_EQ(opt_out ? PreviewsEligibilityReason::HOST_BLACKLISTED
                      : PreviewsEligibilityReason::ALLOWED,
              black_list_->IsLoadedAndAllowed(url2, PreviewsType::OFFLINE,
                                              &passed_reasons_));
  }
}

TEST_F(PreviewsBlackListTest, MaxHosts) {
  // Test that the black list only stores n hosts, and it stores the correct n
  // hosts.
  const GURL url_a("http://www.url_a.com");
  const GURL url_b("http://www.url_b.com");
  const GURL url_c("http://www.url_c.com");
  const GURL url_d("http://www.url_d.com");
  const GURL url_e("http://www.url_e.com");

  // Host indifferent blacklisting should have no effect with the following
  // params.
  const size_t host_indifferent_history = 1;
  const size_t stored_history_length = 1;
  SetHostHistoryParam(stored_history_length);
  SetHostIndifferentHistoryParam(host_indifferent_history);
  SetHostIndifferentThresholdParam(host_indifferent_history + 1);
  SetMaxHostInBlackListParam(2);
  SetHostThresholdParam(stored_history_length);
  SetHostDurationParam(365);
  // Disable single opt out by setting duration to 0.
  SetSingleOptOutDurationParam(0);

  StartTest(true /* null_opt_out */);

  black_list_->AddPreviewNavigation(url_a, true, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_b, false, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_c, false, PreviewsType::OFFLINE);
  // url_a should stay in the map, since it has an opt out time.
  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_a, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_c, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_d, true, PreviewsType::OFFLINE);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_e, true, PreviewsType::OFFLINE);
  // url_d and url_e should remain in the map, but url_a should be evicted.
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_a, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_d, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url_e, PreviewsType::OFFLINE,
                                            &passed_reasons_));
}

TEST_F(PreviewsBlackListTest, SingleOptOut) {
  // Test that when a user opts out of a preview, previews won't be shown until
  // |single_opt_out_duration| has elapsed.
  const GURL url_a("http://www.url_a.com");
  const GURL url_b("http://www.url_b.com");
  const GURL url_c("http://www.url_c.com");

  // Host indifferent blacklisting should have no effect with the following
  // params.
  const size_t host_indifferent_history = 1;
  const int single_opt_out_duration = 5;
  SetHostHistoryParam(1);
  SetHostIndifferentHistoryParam(2);
  SetHostDurationParam(365);
  SetMaxHostInBlackListParam(10);
  SetHostIndifferentHistoryParam(host_indifferent_history);
  SetHostIndifferentThresholdParam(host_indifferent_history + 1);
  SetSingleOptOutDurationParam(single_opt_out_duration);

  StartTest(true /* null_opt_out */);

  black_list_->AddPreviewNavigation(url_a, false, PreviewsType::OFFLINE);
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_a, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_c, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  test_clock_.Advance(
      base::TimeDelta::FromSeconds(single_opt_out_duration + 1));

  black_list_->AddPreviewNavigation(url_b, true, PreviewsType::OFFLINE);
  EXPECT_EQ(PreviewsEligibilityReason::USER_RECENTLY_OPTED_OUT,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::USER_RECENTLY_OPTED_OUT,
            black_list_->IsLoadedAndAllowed(url_c, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  test_clock_.Advance(
      base::TimeDelta::FromSeconds(single_opt_out_duration - 1));

  EXPECT_EQ(PreviewsEligibilityReason::USER_RECENTLY_OPTED_OUT,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::USER_RECENTLY_OPTED_OUT,
            black_list_->IsLoadedAndAllowed(url_c, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  test_clock_.Advance(
      base::TimeDelta::FromSeconds(single_opt_out_duration + 1));

  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_b, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_c, PreviewsType::OFFLINE,
                                            &passed_reasons_));
}

TEST_F(PreviewsBlackListTest, AddPreviewUMA) {
  base::HistogramTester histogram_tester;
  const GURL url("http://www.url.com");

  StartTest(false /* null_opt_out */);

  black_list_->AddPreviewNavigation(url, false, PreviewsType::OFFLINE);
  histogram_tester.ExpectUniqueSample("Previews.OptOut.UserOptedOut.Offline", 0,
                                      1);
  black_list_->AddPreviewNavigation(url, true, PreviewsType::OFFLINE);
  histogram_tester.ExpectBucketCount("Previews.OptOut.UserOptedOut.Offline", 1,
                                     1);
}

TEST_F(PreviewsBlackListTest, ClearShortTime) {
  // Tests that clearing the black list for a short amount of time (relative to
  // "SetSingleOptOutDurationParam") does not reset the blacklist's recent
  // opt out rule.

  const GURL url("http://www.url.com");
  RunClearingBlackListTest(url, true /* short_time */);
  EXPECT_EQ(PreviewsEligibilityReason::USER_RECENTLY_OPTED_OUT,
            black_list_->IsLoadedAndAllowed(url, PreviewsType::OFFLINE,
                                            &passed_reasons_));
}

TEST_F(PreviewsBlackListTest, ClearingBlackListClearsRecentNavigation) {
  // Tests that clearing the black list for a long amount of time (relative to
  // "single_opt_out_duration_in_seconds") resets the blacklist's recent opt out
  // rule.

  const GURL url("http://www.url.com");
  RunClearingBlackListTest(url, false /* short_time */);

  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url, PreviewsType::OFFLINE,
                                            &passed_reasons_));
}

TEST_F(PreviewsBlackListTest, ObserverIsNotifiedOnHostBlacklisted) {
  // Tests the black list behavior when a null OptOutStore is passed in.
  const GURL url_("http://www.url_.com");

  // Host indifferent blacklisting should have no effect with the following
  // params.
  const size_t host_indifferent_history = 1;
  SetHostHistoryParam(4);
  SetHostThresholdParam(2);
  SetHostIndifferentHistoryParam(host_indifferent_history);
  SetHostIndifferentThresholdParam(host_indifferent_history + 1);
  SetHostDurationParam(365);
  // Disable single opt out by setting duration to 0.
  SetSingleOptOutDurationParam(0);

  StartTest(true /* null_opt_out */);

  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url_, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  // Observer is not notified as blacklisted when the threshold does not met.
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_, true, PreviewsType::OFFLINE);
  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(blacklist_delegate_.blacklisted_hosts(), ::testing::SizeIs(0));

  // Observer is notified as blacklisted when the threshold is met.
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_, true, PreviewsType::OFFLINE);
  base::RunLoop().RunUntilIdle();
  const base::Time blacklisted_time = test_clock_.Now();
  EXPECT_THAT(blacklist_delegate_.blacklisted_hosts(), ::testing::SizeIs(1));
  EXPECT_EQ(blacklisted_time,
            blacklist_delegate_.blacklisted_hosts().find(url_.host())->second);

  // Observer is not notified when the host is already blacklisted.
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(url_, true, PreviewsType::OFFLINE);
  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(blacklist_delegate_.blacklisted_hosts(), ::testing::SizeIs(1));
  EXPECT_EQ(blacklisted_time,
            blacklist_delegate_.blacklisted_hosts().find(url_.host())->second);

  // Observer is notified when blacklist is cleared.
  EXPECT_FALSE(blacklist_delegate_.blacklist_cleared());

  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->ClearBlackList(start_, test_clock_.Now());
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(blacklist_delegate_.blacklist_cleared());
  EXPECT_EQ(test_clock_.Now(), blacklist_delegate_.blacklist_cleared_time());
}

TEST_F(PreviewsBlackListTest, ObserverIsNotifiedOnUserBlacklisted) {
  // Tests the black list behavior when a null OptOutStore is passed in.
  const GURL urls[] = {
      GURL("http://www.url_0.com"), GURL("http://www.url_1.com"),
      GURL("http://www.url_2.com"), GURL("http://www.url_3.com"),
  };

  // Per host blacklisting should have no effect with the following params.
  const size_t per_host_history = 1;
  const size_t host_indifferent_history = 4;
  const size_t host_indifferent_threshold = host_indifferent_history;
  SetHostHistoryParam(per_host_history);
  SetHostIndifferentHistoryParam(host_indifferent_history);
  SetHostThresholdParam(per_host_history + 1);
  SetHostIndifferentThresholdParam(host_indifferent_threshold);
  SetHostDurationParam(365);
  // Disable single opt out by setting duration to 0.
  SetSingleOptOutDurationParam(0);

  StartTest(true /* null_opt_out */);

  // Initially no host is blacklisted, and user is not blacklisted.
  EXPECT_THAT(blacklist_delegate_.blacklisted_hosts(), ::testing::SizeIs(0));
  EXPECT_FALSE(blacklist_delegate_.user_blacklisted());

  for (size_t i = 0; i < host_indifferent_threshold; ++i) {
    test_clock_.Advance(base::TimeDelta::FromSeconds(1));
    black_list_->AddPreviewNavigation(urls[i], true, PreviewsType::OFFLINE);
    base::RunLoop().RunUntilIdle();

    EXPECT_THAT(blacklist_delegate_.blacklisted_hosts(), ::testing::SizeIs(0));
    // Observer is notified when number of recently opt out meets
    // |host_indifferent_threshold|.
    EXPECT_EQ(i >= host_indifferent_threshold - 1,
              blacklist_delegate_.user_blacklisted());
  }

  // Observer is notified when the user is no longer blacklisted.
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));
  black_list_->AddPreviewNavigation(urls[3], false, PreviewsType::OFFLINE);
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(blacklist_delegate_.user_blacklisted());
}

TEST_F(PreviewsBlackListTest, ObserverIsNotifiedWhenLoadBlacklistDone) {
  const GURL url_a1("http://www.url_a.com/a1");
  const GURL url_a2("http://www.url_a.com/a2");

  // Per host blacklisting should have no effect with the following params.
  const size_t per_host_history = 1;
  const size_t host_indifferent_history = 4;
  const size_t host_indifferent_threshold = host_indifferent_history;
  SetHostHistoryParam(per_host_history);
  SetHostIndifferentHistoryParam(host_indifferent_history);
  SetHostThresholdParam(per_host_history + 1);
  SetHostIndifferentThresholdParam(host_indifferent_threshold);
  SetHostDurationParam(365);
  // Disable single opt out by setting duration to 0.
  SetSingleOptOutDurationParam(0);

  StartTest(false /* null_opt_out */);

  std::unique_ptr<PreviewsBlackListItem> host_indifferent_item =
      PreviewsBlackList::CreateHostIndifferentBlackListItem();
  base::SimpleTestClock test_clock;

  for (size_t i = 0; i < host_indifferent_threshold; ++i) {
    test_clock.Advance(base::TimeDelta::FromSeconds(1));
    host_indifferent_item->AddPreviewNavigation(true, test_clock.Now());
  }

  std::unique_ptr<TestPreviewsOptOutStore> opt_out_store =
      std::make_unique<TestPreviewsOptOutStore>();
  opt_out_store->SetHostIndifferentBlacklistItem(
      std::move(host_indifferent_item));

  EXPECT_FALSE(blacklist_delegate_.user_blacklisted());
  auto black_list = std::make_unique<PreviewsBlackList>(
      std::move(opt_out_store), &test_clock, &blacklist_delegate_);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(blacklist_delegate_.user_blacklisted());
}

TEST_F(PreviewsBlackListTest, ObserverIsNotifiedOfHistoricalBlacklistedHosts) {
  // Tests the black list behavior when a non-null OptOutStore is passed in.
  const GURL url_a("http://www.url_a.com");
  const GURL url_b("http://www.url_b.com");

  // Host indifferent blacklisting should have no effect with the following
  // params.
  const size_t host_indifferent_history = 1;
  SetHostThresholdParam(2);
  SetHostHistoryParam(4);
  SetHostIndifferentHistoryParam(host_indifferent_history);
  SetHostIndifferentThresholdParam(host_indifferent_history + 1);
  SetHostDurationParam(365);
  // Disable single opt out by setting duration to 0.
  SetSingleOptOutDurationParam(0);

  StartTest(false /* null_opt_out */);

  base::SimpleTestClock test_clock;

  PreviewsBlackListItem* item_a = new PreviewsBlackListItem(
      params::MaxStoredHistoryLengthForPerHostBlackList(),
      params::PerHostBlackListOptOutThreshold(),
      params::PerHostBlackListDuration());
  PreviewsBlackListItem* item_b = new PreviewsBlackListItem(
      params::MaxStoredHistoryLengthForPerHostBlackList(),
      params::PerHostBlackListOptOutThreshold(),
      params::PerHostBlackListDuration());

  // Host |url_a| is blacklisted.
  test_clock.Advance(base::TimeDelta::FromSeconds(1));
  item_a->AddPreviewNavigation(true, test_clock.Now());
  test_clock.Advance(base::TimeDelta::FromSeconds(1));
  item_a->AddPreviewNavigation(true, test_clock.Now());
  base::Time blacklisted_time = test_clock.Now();

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(item_a->IsBlackListed(test_clock.Now()));

  // Host |url_b| is not blacklisted.
  test_clock.Advance(base::TimeDelta::FromSeconds(1));
  item_b->AddPreviewNavigation(true, test_clock.Now());

  std::unique_ptr<BlackListItemMap> item_map =
      std::make_unique<BlackListItemMap>();
  item_map->emplace(url_a.host(), base::WrapUnique(item_a));
  item_map->emplace(url_b.host(), base::WrapUnique(item_b));

  std::unique_ptr<TestPreviewsOptOutStore> opt_out_store =
      std::make_unique<TestPreviewsOptOutStore>();
  opt_out_store->SetBlacklistItemMap(std::move(item_map));

  auto black_list = std::make_unique<PreviewsBlackList>(
      std::move(opt_out_store), &test_clock, &blacklist_delegate_);
  base::RunLoop().RunUntilIdle();

  ASSERT_THAT(blacklist_delegate_.blacklisted_hosts(), ::testing::SizeIs(1));
  EXPECT_EQ(blacklisted_time,
            blacklist_delegate_.blacklisted_hosts().find(url_a.host())->second);
}

TEST_F(PreviewsBlackListTest, PassedReasonsWhenBlacklistDataNotLoaded) {
  // Test that IsLoadedAndAllow, push checked PreviewsEligibilityReasons to the
  // |passed_reasons| vector.
  const GURL url("http://www.url_.com/");
  StartTest(false /* null_opt_out */);

  EXPECT_EQ(PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED,
            black_list_->IsLoadedAndAllowed(url, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  EXPECT_EQ(0UL, passed_reasons_.size());
}

TEST_F(PreviewsBlackListTest, PassedReasonsWhenUserRecentlyOptedOut) {
  // Test that IsLoadedAndAllow, push checked PreviewsEligibilityReasons to the
  // |passed_reasons| vector.
  const GURL url("http://www.url_.com/");
  StartTest(true /* null_opt_out */);

  black_list_->AddPreviewNavigation(url, true, PreviewsType::OFFLINE);
  EXPECT_EQ(PreviewsEligibilityReason::USER_RECENTLY_OPTED_OUT,
            black_list_->IsLoadedAndAllowed(url, PreviewsType::OFFLINE,
                                            &passed_reasons_));
  EXPECT_EQ(1UL, passed_reasons_.size());
  EXPECT_EQ(PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED,
            passed_reasons_[0]);
}

TEST_F(PreviewsBlackListTest, PassedReasonsWhenUserBlacklisted) {
  // Test that IsLoadedAndAllow, push checked PreviewsEligibilityReasons to the
  // |passed_reasons| vector.
  const GURL urls[] = {
      GURL("http://www.url_0.com"), GURL("http://www.url_1.com"),
      GURL("http://www.url_2.com"), GURL("http://www.url_3.com"),
  };

  // Per host blacklisting should have no effect with the following params.
  const size_t per_host_history = 1;
  const size_t host_indifferent_history = 4;
  const size_t host_indifferent_threshold = host_indifferent_history;
  SetHostHistoryParam(per_host_history);
  SetHostIndifferentHistoryParam(host_indifferent_history);
  SetHostThresholdParam(per_host_history + 1);
  SetHostIndifferentThresholdParam(host_indifferent_threshold);
  SetHostDurationParam(365);
  // Disable single opt out by setting duration to 0.
  SetSingleOptOutDurationParam(0);

  StartTest(true /* null_opt_out */);
  test_clock_.Advance(base::TimeDelta::FromSeconds(1));

  for (auto url : urls) {
    black_list_->AddPreviewNavigation(url, true, PreviewsType::OFFLINE);
  }

  EXPECT_EQ(PreviewsEligibilityReason::USER_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(urls[0], PreviewsType::OFFLINE,
                                            &passed_reasons_));

  PreviewsEligibilityReason expected_reasons[] = {
      PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED,
      PreviewsEligibilityReason::USER_RECENTLY_OPTED_OUT,
  };
  EXPECT_EQ(2UL, passed_reasons_.size());
  for (size_t i = 0; i < passed_reasons_.size(); i++) {
    EXPECT_EQ(expected_reasons[i], passed_reasons_[i]);
  }
}

TEST_F(PreviewsBlackListTest, PassedReasonsWhenHostBlacklisted) {
  // Test that IsLoadedAndAllow, push checked PreviewsEligibilityReasons to the
  // |passed_reasons| vector.
  const GURL url("http://www.url_a.com");

  // Host indifferent blacklisting should have no effect with the following
  // params.
  const size_t host_indifferent_history = 1;
  SetHostHistoryParam(4);
  SetHostIndifferentHistoryParam(host_indifferent_history);
  SetHostThresholdParam(2);
  SetHostIndifferentThresholdParam(host_indifferent_history + 1);
  SetHostDurationParam(365);
  // Disable single opt out by setting duration to 0.
  SetSingleOptOutDurationParam(0);

  StartTest(true /* null_opt_out */);

  black_list_->AddPreviewNavigation(url, true, PreviewsType::OFFLINE);
  black_list_->AddPreviewNavigation(url, true, PreviewsType::OFFLINE);

  EXPECT_EQ(PreviewsEligibilityReason::HOST_BLACKLISTED,
            black_list_->IsLoadedAndAllowed(url, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  PreviewsEligibilityReason expected_reasons[] = {
      PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED,
      PreviewsEligibilityReason::USER_RECENTLY_OPTED_OUT,
      PreviewsEligibilityReason::USER_BLACKLISTED,
  };
  EXPECT_EQ(3UL, passed_reasons_.size());
  for (size_t i = 0; i < passed_reasons_.size(); i++) {
    EXPECT_EQ(expected_reasons[i], passed_reasons_[i]);
  }
}

TEST_F(PreviewsBlackListTest, PassedReasonsWhenAllowed) {
  // Test that IsLoadedAndAllow, push checked PreviewsEligibilityReasons to the
  // |passed_reasons| vector.
  const GURL url("http://www.url.com");
  StartTest(true /* null_opt_out */);

  EXPECT_EQ(PreviewsEligibilityReason::ALLOWED,
            black_list_->IsLoadedAndAllowed(url, PreviewsType::OFFLINE,
                                            &passed_reasons_));

  PreviewsEligibilityReason expected_reasons[] = {
      PreviewsEligibilityReason::BLACKLIST_DATA_NOT_LOADED,
      PreviewsEligibilityReason::USER_RECENTLY_OPTED_OUT,
      PreviewsEligibilityReason::USER_BLACKLISTED,
      PreviewsEligibilityReason::HOST_BLACKLISTED,
  };
  EXPECT_EQ(4UL, passed_reasons_.size());
  for (size_t i = 0; i < passed_reasons_.size(); i++) {
    EXPECT_EQ(expected_reasons[i], passed_reasons_[i]);
  }
}

}  // namespace

}  // namespace previews

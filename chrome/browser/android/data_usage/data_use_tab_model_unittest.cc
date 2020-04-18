// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/data_usage/data_use_tab_model.h"

#include <stdint.h>

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string16.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/time/time.h"
#include "chrome/browser/android/data_usage/data_use_matcher.h"
#include "chrome/browser/android/data_usage/external_data_use_observer_bridge.h"
#include "chrome/browser/android/data_usage/tab_data_use_entry.h"
#include "components/data_usage/core/data_use_aggregator.h"
#include "components/data_usage/core/data_use_amortizer.h"
#include "components/data_usage/core/data_use_annotator.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/network_change_notifier.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

// Tracking labels for tests.
const char kTestLabel1[] = "label_1";
const char kTestLabel2[] = "label_2";
const char kTestLabel3[] = "label_3";

const SessionID kTabID1 = SessionID::FromSerializedValue(1);
const SessionID kTabID2 = SessionID::FromSerializedValue(2);
const SessionID kTabID3 = SessionID::FromSerializedValue(3);

const char kURLFoo[] = "http://foo.com/";
const char kURLBar[] = "http://bar.com/";
const char kURLBaz[] = "http://baz.com/";
const char kURLFooBar[] = "http://foobar.com/";
const char kPackageFoo[] = "com.google.package.foo";
const char kPackageBar[] = "com.google.package.bar";

const char kDataUseTabModelLabel[] = "data_use_tab_model_label";

enum TabEntrySize { ZERO = 0, ONE, TWO, THREE };

// Indicates the tracking states for a sequence of navigations.
enum TrackingState {
  NONE,
  STARTED,
  ENDED,
  CONTINUES,
};

// Mock observer to track the calls to start and end tracking events.
class MockTabDataUseObserver
    : public android::DataUseTabModel::TabDataUseObserver {
 public:
  MOCK_METHOD1(NotifyTrackingStarting, void(SessionID tab_id));
  MOCK_METHOD1(NotifyTrackingEnding, void(SessionID tab_id));
  MOCK_METHOD0(OnDataUseTabModelReady, void());
};

class TestExternalDataUseObserverBridge
    : public android::ExternalDataUseObserverBridge {
 public:
  TestExternalDataUseObserverBridge() {}
  void FetchMatchingRules() const override {}
  void ShouldRegisterAsDataUseObserver(bool should_register) const override {}
};

std::unique_ptr<content::NavigationEntry> CreateNavigationEntry(
    const std::string& url) {
  auto navigation_entry(content::NavigationEntry::Create());
  navigation_entry->SetURL(GURL(url));
  return navigation_entry;
}

void ExpectDataUseLabelInNavigationEntry(
    const content::NavigationEntry& navigation_entry,
    const std::string& label) {
  base::string16 extra_data;
  bool exists =
      navigation_entry.GetExtraData(kDataUseTabModelLabel, &extra_data);
  EXPECT_EQ(!label.empty(), exists);
  EXPECT_EQ(label, base::UTF16ToASCII(extra_data));
}

}  // namespace

namespace android {

class DataUseTabModelTest : public testing::Test {
 public:
  DataUseTabModelTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        external_data_use_observer_bridge_(
            new TestExternalDataUseObserverBridge()) {}

 protected:
  void SetUp() override {
    base::RunLoop().RunUntilIdle();
    data_use_tab_model_.reset(new DataUseTabModel(
        base::Bind(&ExternalDataUseObserverBridge::FetchMatchingRules,
                   base::Unretained(external_data_use_observer_bridge_.get())),
        base::Bind(
            &ExternalDataUseObserverBridge::ShouldRegisterAsDataUseObserver,
            base::Unretained(external_data_use_observer_bridge_.get()))));

    // Advance to non nil time.
    tick_clock_.Advance(base::TimeDelta::FromSeconds(1));

    data_use_tab_model_->tick_clock_ = &tick_clock_;
    data_use_tab_model_->OnControlAppInstallStateChange(true);
  }

  // Returns true if tab entry for |tab_id| exists in |active_tabs_|.
  bool IsTabEntryExists(SessionID tab_id) const {
    return data_use_tab_model_->active_tabs_.find(tab_id) !=
           data_use_tab_model_->active_tabs_.end();
  }

  // Checks if there are |expected_size| tab entries being tracked in
  // |active_tabs_|.
  void ExpectTabEntrySize(uint32_t expected_size) const {
    EXPECT_EQ(expected_size, data_use_tab_model_->active_tabs_.size());
  }

  // Returns true if |tab_id| is a custom tab and started tracking due to
  // package match.
  bool IsCustomTabPackageMatch(SessionID tab_id) const {
    auto tab_entry_iterator = data_use_tab_model_->active_tabs_.find(tab_id);
    return (tab_entry_iterator != data_use_tab_model_->active_tabs_.end()) &&
           tab_entry_iterator->second.is_custom_tab_package_match();
  }

  // Returns true if the tracking session for tab with id |tab_id| is currently
  // active.
  bool IsTrackingDataUse(SessionID tab_id) const {
    const auto& tab_entry_iterator =
        data_use_tab_model_->active_tabs_.find(tab_id);
    if (tab_entry_iterator == data_use_tab_model_->active_tabs_.end())
      return false;
    return tab_entry_iterator->second.IsTrackingDataUse();
  }

  // Checks if the DataUse object for the given |tab_id| with request start time
  // |at_time| is labeled as an empty tracking info.
  void ExpectEmptyTrackingInfoAtTime(SessionID tab_id,
                                     const base::TimeTicks& at_time) const {
    ExpectTrackingInfoAtTimeWithReturn(tab_id, at_time, false,
                                       DataUseTabModel::TrackingInfo());
  }

  // Checks if the DataUse object for the given |tab_id| is labeled as an empty
  // tracking info.
  void ExpectEmptyTrackingInfo(SessionID tab_id) {
    ExpectTrackingInfoAtTimeWithReturn(tab_id, tick_clock_.NowTicks(), false,
                                       DataUseTabModel::TrackingInfo());
  }

  // Checks if the DataUse object for given |tab_id| is labeled as
  // |expected_label| with custom tab indicated by |expected_is_custom_tab|.
  void ExpectTrackingInfo(SessionID tab_id,
                          const std::string& expected_label,
                          const std::string& expected_tag) {
    DataUseTabModel::TrackingInfo expected_tracking_info;
    expected_tracking_info.label = expected_label;
    expected_tracking_info.tag = expected_tag;
    ExpectTrackingInfoAtTimeWithReturn(tab_id, tick_clock_.NowTicks(),
                                       !expected_label.empty(),
                                       expected_tracking_info);
  }

  // Checks if GetTrackingInfoForTabAtTime returns the tracking info for the
  // DataUse object for |tab_id| with request start time |at_time|, as
  // |expected_tracking_info| and returns |expected_return|.
  void ExpectTrackingInfoAtTimeWithReturn(
      SessionID tab_id,
      const base::TimeTicks& at_time,
      bool expected_return,
      const DataUseTabModel::TrackingInfo& expected_tracking_info) const {
    DataUseTabModel::TrackingInfo actual_tracking_info;
    bool actual_return = data_use_tab_model_->GetTrackingInfoForTabAtTime(
        tab_id, at_time, &actual_tracking_info);
    EXPECT_EQ(expected_return, actual_return);
    if (expected_return) {
      EXPECT_EQ(expected_tracking_info.label, actual_tracking_info.label);
      EXPECT_EQ(expected_tracking_info.tag, actual_tracking_info.tag);
    }
  }

  void StartTrackingDataUse(SessionID tab_id, const std::string& label) {
    data_use_tab_model_->StartTrackingDataUse(
        DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, tab_id, label, false);
  }

  void EndTrackingDataUse(SessionID tab_id) {
    data_use_tab_model_->EndTrackingDataUse(
        DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, tab_id);
  }

  void RegisterURLRegexes(const std::vector<std::string>& app_package_names,
                          const std::vector<std::string>& domain_regexes,
                          const std::vector<std::string>& labels) {
    data_use_tab_model_->RegisterURLRegexes(app_package_names, domain_regexes,
                                            labels);
  }

  base::SimpleTestTickClock tick_clock_;

  std::unique_ptr<DataUseTabModel> data_use_tab_model_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<ExternalDataUseObserverBridge>
      external_data_use_observer_bridge_;

  DISALLOW_COPY_AND_ASSIGN(DataUseTabModelTest);
};

// Starts and ends tracking a single tab and checks if its label is returned
// correctly for DataUse objects.
TEST_F(DataUseTabModelTest, SingleTabTracking) {
  ExpectTabEntrySize(TabEntrySize::ZERO);

  // No label is applied initially.
  ExpectEmptyTrackingInfo(kTabID1);
  ExpectEmptyTrackingInfo(kTabID2);

  StartTrackingDataUse(kTabID1, kTestLabel1);
  ExpectTabEntrySize(TabEntrySize::ONE);

  EXPECT_TRUE(IsTrackingDataUse(kTabID1));
  ExpectTrackingInfo(kTabID1, kTestLabel1, DataUseTabModel::kDefaultTag);
  ExpectEmptyTrackingInfo(kTabID2);

  EndTrackingDataUse(kTabID1);
  ExpectTabEntrySize(TabEntrySize::ONE);
  EXPECT_FALSE(IsTrackingDataUse(kTabID1));
}

// Starts and ends tracking multiple tabs and checks if labels are returned
// correctly for DataUse objects corresponding to different tab ids.
TEST_F(DataUseTabModelTest, MultipleTabTracking) {
  ExpectTabEntrySize(TabEntrySize::ZERO);
  ExpectEmptyTrackingInfo(kTabID1);
  ExpectEmptyTrackingInfo(kTabID2);
  ExpectEmptyTrackingInfo(kTabID3);

  StartTrackingDataUse(kTabID1, kTestLabel1);
  StartTrackingDataUse(kTabID2, kTestLabel2);
  StartTrackingDataUse(kTabID3, kTestLabel3);
  ExpectTabEntrySize(TabEntrySize::THREE);

  EXPECT_TRUE(IsTrackingDataUse(kTabID1));
  EXPECT_TRUE(IsTrackingDataUse(kTabID2));
  EXPECT_TRUE(IsTrackingDataUse(kTabID3));
  ExpectTrackingInfo(kTabID1, kTestLabel1, DataUseTabModel::kDefaultTag);
  ExpectTrackingInfo(kTabID2, kTestLabel2, DataUseTabModel::kDefaultTag);
  ExpectTrackingInfo(kTabID3, kTestLabel3, DataUseTabModel::kDefaultTag);

  EndTrackingDataUse(kTabID1);
  EndTrackingDataUse(kTabID2);
  EndTrackingDataUse(kTabID3);
  ExpectTabEntrySize(TabEntrySize::THREE);
  EXPECT_FALSE(IsTrackingDataUse(kTabID1));
  EXPECT_FALSE(IsTrackingDataUse(kTabID2));
  EXPECT_FALSE(IsTrackingDataUse(kTabID3));

  // Future data use object should be labeled as an empty string.
  base::TimeTicks future_time =
      tick_clock_.NowTicks() + base::TimeDelta::FromMilliseconds(20);
  ExpectEmptyTrackingInfoAtTime(kTabID1, future_time);
  ExpectEmptyTrackingInfoAtTime(kTabID2, future_time);
  ExpectEmptyTrackingInfoAtTime(kTabID3, future_time);
}

// Checks that the mock observer receives start and end tracking events for a
// single tab.
TEST_F(DataUseTabModelTest, ObserverStartEndEvents) {
  MockTabDataUseObserver mock_observer;

  EXPECT_CALL(mock_observer, NotifyTrackingStarting(kTabID1)).Times(1);
  EXPECT_CALL(mock_observer, NotifyTrackingEnding(kTabID1)).Times(1);

  data_use_tab_model_->AddObserver(&mock_observer);
  StartTrackingDataUse(kTabID1, kTestLabel1);
  EndTrackingDataUse(kTabID1);
}

// Checks that multiple mock observers receive start and end tracking events for
// multiple tabs.
TEST_F(DataUseTabModelTest, MultipleObserverMultipleStartEndEvents) {
  const int kMaxMockObservers = 5;
  MockTabDataUseObserver mock_observers[kMaxMockObservers];

  for (auto& mock_observer : mock_observers) {
    // Add the observer.
    data_use_tab_model_->AddObserver(&mock_observer);

    // Expect start and end events for tab ids 1-3.
    EXPECT_CALL(mock_observer, NotifyTrackingStarting(kTabID1)).Times(1);
    EXPECT_CALL(mock_observer, NotifyTrackingEnding(kTabID1)).Times(1);
    EXPECT_CALL(mock_observer, NotifyTrackingStarting(kTabID2)).Times(1);
    EXPECT_CALL(mock_observer, NotifyTrackingEnding(kTabID2)).Times(1);
    EXPECT_CALL(mock_observer, NotifyTrackingStarting(kTabID3)).Times(1);
    EXPECT_CALL(mock_observer, NotifyTrackingEnding(kTabID3)).Times(1);
  }

  // Start and end tracking for tab ids 1-3.
  StartTrackingDataUse(kTabID1, kTestLabel1);
  StartTrackingDataUse(kTabID2, kTestLabel2);
  StartTrackingDataUse(kTabID3, kTestLabel3);
  EndTrackingDataUse(kTabID1);
  EndTrackingDataUse(kTabID2);
  EndTrackingDataUse(kTabID3);
}

// Checks that the observer is not notified of start and end events after
// RemoveObserver.
TEST_F(DataUseTabModelTest, ObserverNotNotifiedAfterRemove) {
  MockTabDataUseObserver mock_observer;

  // Observer notified of start and end events.
  EXPECT_CALL(mock_observer, NotifyTrackingStarting(kTabID1)).Times(1);
  EXPECT_CALL(mock_observer, NotifyTrackingEnding(kTabID1)).Times(1);

  data_use_tab_model_->AddObserver(&mock_observer);
  StartTrackingDataUse(kTabID1, kTestLabel1);
  EndTrackingDataUse(kTabID1);

  testing::Mock::VerifyAndClear(&mock_observer);

  // Observer should not be notified after RemoveObserver.
  EXPECT_CALL(mock_observer, NotifyTrackingStarting(kTabID1)).Times(0);
  EXPECT_CALL(mock_observer, NotifyTrackingEnding(kTabID1)).Times(0);

  data_use_tab_model_->RemoveObserver(&mock_observer);
  StartTrackingDataUse(kTabID1, kTestLabel1);
  EndTrackingDataUse(kTabID1);
}

// Checks that tab close event updates the close time of the tab entry.
TEST_F(DataUseTabModelTest, TabCloseEvent) {
  StartTrackingDataUse(kTabID1, kTestLabel1);
  EndTrackingDataUse(kTabID1);

  ExpectTabEntrySize(TabEntrySize::ONE);
  EXPECT_TRUE(data_use_tab_model_->active_tabs_.find(kTabID1)
                  ->second.tab_close_time_.is_null());

  data_use_tab_model_->OnTabCloseEvent(kTabID1);

  ExpectTabEntrySize(TabEntrySize::ONE);
  EXPECT_FALSE(data_use_tab_model_->active_tabs_.find(kTabID1)
                   ->second.tab_close_time_.is_null());
}

// Checks that tab close event ends the active tracking session for the tab.
TEST_F(DataUseTabModelTest, TabCloseEventEndsTracking) {
  StartTrackingDataUse(kTabID1, kTestLabel1);
  EXPECT_TRUE(IsTrackingDataUse(kTabID1));

  data_use_tab_model_->OnTabCloseEvent(kTabID1);
  EXPECT_FALSE(IsTrackingDataUse(kTabID1));

  // Future data use object should be labeled as an empty string.
  ExpectEmptyTrackingInfoAtTime(
      kTabID1, tick_clock_.NowTicks() + base::TimeDelta::FromMilliseconds(20));
}

// Checks that end tracking for specific labels closes those active sessions.
TEST_F(DataUseTabModelTest, OnTrackingLabelRemoved) {
  MockTabDataUseObserver mock_observer;

  StartTrackingDataUse(kTabID1, kTestLabel1);
  StartTrackingDataUse(kTabID2, kTestLabel2);
  StartTrackingDataUse(kTabID3, kTestLabel3);
  data_use_tab_model_->AddObserver(&mock_observer);
  ExpectTabEntrySize(TabEntrySize::THREE);

  EXPECT_TRUE(IsTrackingDataUse(kTabID1));
  EXPECT_TRUE(IsTrackingDataUse(kTabID2));
  EXPECT_TRUE(IsTrackingDataUse(kTabID3));

  // Observer not notified of end tracking.
  EXPECT_CALL(mock_observer, NotifyTrackingEnding(kTabID2)).Times(0);

  data_use_tab_model_->OnTrackingLabelRemoved(kTestLabel2);

  EXPECT_TRUE(IsTrackingDataUse(kTabID1));
  EXPECT_FALSE(IsTrackingDataUse(kTabID2));
  EXPECT_TRUE(IsTrackingDataUse(kTabID3));

  // Observer not notified of end tracking.
  EXPECT_CALL(mock_observer, NotifyTrackingEnding(kTabID3)).Times(0);

  data_use_tab_model_->OnTrackingLabelRemoved(kTestLabel3);

  EXPECT_TRUE(IsTrackingDataUse(kTabID1));
  EXPECT_FALSE(IsTrackingDataUse(kTabID2));
  EXPECT_FALSE(IsTrackingDataUse(kTabID3));
}

// Checks that |active_tabs_| does not grow beyond GetMaxTabEntriesForTests tab
// entries.
TEST_F(DataUseTabModelTest, CompactTabEntriesWithinMaxLimit) {
  const int32_t max_tab_entries =
      static_cast<int32_t>(data_use_tab_model_->max_tab_entries_);
  std::list<SessionID> tab_ids;

  ExpectTabEntrySize(TabEntrySize::ZERO);

  for (int i = 0; i < max_tab_entries; ++i) {
    SessionID tab_id = SessionID::NewUnique();
    tab_ids.push_back(tab_id);
    std::string tab_label = base::StringPrintf("label_%d", tab_id.id());
    StartTrackingDataUse(tab_id, tab_label);
    tick_clock_.Advance(base::TimeDelta::FromSeconds(1));
    EndTrackingDataUse(tab_id);
    tick_clock_.Advance(base::TimeDelta::FromSeconds(1));

    ExpectTabEntrySize(i + 1);
  }

  // Starting and ending more tracking tab entries does not increase the size of
  // |active_tabs_|.
  for (int i = 0; i < 10; ++i) {
    SessionID oldest_tab_id = tab_ids.front();

    EXPECT_TRUE(IsTabEntryExists(oldest_tab_id));
    SessionID tab_id = SessionID::NewUnique();
    tab_ids.push_back(tab_id);
    std::string tab_label = base::StringPrintf("label_%d", tab_id.id());
    StartTrackingDataUse(tab_id, tab_label);
    tick_clock_.Advance(base::TimeDelta::FromSeconds(1));
    EndTrackingDataUse(tab_id);
    tick_clock_.Advance(base::TimeDelta::FromSeconds(1));

    // Oldest entry got removed.
    EXPECT_FALSE(IsTabEntryExists(oldest_tab_id));
    ExpectTabEntrySize(max_tab_entries);

    tab_ids.pop_front();  // next oldest tab entry.
  }

  // Starting and not ending more tracking tab entries does not increase the
  // size of |active_tabs_|.
  for (int i = 0; i < 10; ++i) {
    SessionID oldest_tab_id = tab_ids.front();
    EXPECT_TRUE(IsTabEntryExists(oldest_tab_id));
    SessionID tab_id = SessionID::NewUnique();
    tab_ids.push_back(tab_id);
    std::string tab_label = base::StringPrintf("label_%d", tab_id.id());
    StartTrackingDataUse(tab_id, tab_label);
    tick_clock_.Advance(base::TimeDelta::FromSeconds(1));

    // Oldest entry got removed.
    EXPECT_FALSE(IsTabEntryExists(oldest_tab_id));
    ExpectTabEntrySize(max_tab_entries);

    tab_ids.pop_front();  // next oldest tab entry.
  }
}

TEST_F(DataUseTabModelTest, ExpiredInactiveTabEntryRemovaltimeHistogram) {
  const char kUMAExpiredInactiveTabEntryRemovalDurationHistogram[] =
      "DataUsage.TabModel.ExpiredInactiveTabEntryRemovalDuration";
  base::HistogramTester histogram_tester;

  StartTrackingDataUse(kTabID1, kTestLabel1);
  EndTrackingDataUse(kTabID1);
  EXPECT_FALSE(IsTrackingDataUse(kTabID1));
  data_use_tab_model_->OnTabCloseEvent(kTabID1);

  // Fake tab close time to make it as expired.
  EXPECT_TRUE(IsTabEntryExists(kTabID1));
  auto& tab_entry = data_use_tab_model_->active_tabs_.find(kTabID1)->second;
  EXPECT_FALSE(tab_entry.tab_close_time_.is_null());
  tab_entry.tab_close_time_ -=
      data_use_tab_model_->closed_tab_expiration_duration() +
      base::TimeDelta::FromSeconds(1);
  EXPECT_TRUE(tab_entry.IsExpired());

  // Fast forward 50 seconds.
  tick_clock_.Advance(base::TimeDelta::FromSeconds(50));

  data_use_tab_model_->CompactTabEntries();
  EXPECT_FALSE(IsTabEntryExists(kTabID1));

  histogram_tester.ExpectUniqueSample(
      kUMAExpiredInactiveTabEntryRemovalDurationHistogram,
      base::TimeDelta::FromSeconds(50).InMilliseconds(), 1);
}

TEST_F(DataUseTabModelTest, UnexpiredTabEntryRemovaltimeHistogram) {
  const char kUMAUnexpiredTabEntryRemovalDurationHistogram[] =
      "DataUsage.TabModel.UnexpiredTabEntryRemovalDuration";
  base::HistogramTester histogram_tester;
  const int32_t max_tab_entries =
      static_cast<int32_t>(data_use_tab_model_->max_tab_entries_);

  for (int i = 0; i < max_tab_entries; ++i) {
    SessionID tab_id = SessionID::NewUnique();
    std::string tab_label = base::StringPrintf("label_%d", tab_id.id());
    StartTrackingDataUse(tab_id, tab_label);
    EndTrackingDataUse(tab_id);
  }

  // Fast forward 10 minutes.
  tick_clock_.Advance(base::TimeDelta::FromMinutes(10));

  // Adding another tab entry triggers CompactTabEntries.
  SessionID tab_id = SessionID::NewUnique();
  std::string tab_label = base::StringPrintf("label_%d", tab_id.id());
  StartTrackingDataUse(tab_id, tab_label);
  EndTrackingDataUse(tab_id);

  histogram_tester.ExpectUniqueSample(
      kUMAUnexpiredTabEntryRemovalDurationHistogram,
      base::TimeDelta::FromMinutes(10).InMilliseconds(), 1);
}

// Tests that Enter navigation events start tracking the tab entry.
TEST_F(DataUseTabModelTest, NavigationEnterEvent) {
  std::vector<std::string> app_package_names, domain_regexes, labels;

  // Matching rule with app package name.
  app_package_names.push_back(std::string());
  domain_regexes.push_back(kURLFoo);
  labels.push_back(kTestLabel1);

  // Matching rule with regex.
  app_package_names.push_back(std::string());
  domain_regexes.push_back(kURLBar);
  labels.push_back(kTestLabel2);

  RegisterURLRegexes(app_package_names, domain_regexes, labels);

  ExpectTabEntrySize(TabEntrySize::ZERO);

  auto navigation_entry = CreateNavigationEntry(kURLFoo);
  EXPECT_FALSE(data_use_tab_model_->WouldNavigationEventEndTracking(
      kTabID1, DataUseTabModel::TRANSITION_OMNIBOX_NAVIGATION, GURL(kURLFoo),
      navigation_entry.get()));
  data_use_tab_model_->OnNavigationEvent(
      kTabID1, DataUseTabModel::TRANSITION_OMNIBOX_NAVIGATION, GURL(kURLFoo),
      std::string(), navigation_entry.get());
  ExpectTabEntrySize(TabEntrySize::ONE);
  EXPECT_TRUE(IsTrackingDataUse(kTabID1));
  ExpectTrackingInfo(kTabID1, kTestLabel1, DataUseTabModel::kDefaultTag);
  ExpectDataUseLabelInNavigationEntry(*navigation_entry, kTestLabel1);
  EXPECT_TRUE(data_use_tab_model_->WouldNavigationEventEndTracking(
      kTabID1, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, GURL(kURLFooBar),
      navigation_entry.get()));

  navigation_entry = CreateNavigationEntry(kURLBar);
  EXPECT_FALSE(data_use_tab_model_->WouldNavigationEventEndTracking(
      kTabID2, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, GURL(kURLBar),
      navigation_entry.get()));
  data_use_tab_model_->OnNavigationEvent(
      kTabID2, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, GURL(kURLBar),
      std::string(), navigation_entry.get());
  ExpectTabEntrySize(TabEntrySize::TWO);
  EXPECT_TRUE(IsTrackingDataUse(kTabID2));
  ExpectTrackingInfo(kTabID2, kTestLabel2, DataUseTabModel::kDefaultTag);
  ExpectDataUseLabelInNavigationEntry(*navigation_entry, kTestLabel2);
  EXPECT_TRUE(data_use_tab_model_->WouldNavigationEventEndTracking(
      kTabID2, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, GURL(kURLFooBar),
      navigation_entry.get()));
}

// Tests that a navigation event with empty url and empty package name does not
// start tracking.
TEST_F(DataUseTabModelTest, EmptyNavigationEvent) {
  ExpectTabEntrySize(TabEntrySize::ZERO);

  RegisterURLRegexes(std::vector<std::string>(1, std::string()),
                     std::vector<std::string>(1, kURLFoo),
                     std::vector<std::string>(1, kTestLabel1));

  data_use_tab_model_->OnNavigationEvent(kTabID1,
                                         DataUseTabModel::TRANSITION_CUSTOM_TAB,
                                         GURL(), std::string(), nullptr);
  EXPECT_FALSE(IsTrackingDataUse(kTabID1));

  data_use_tab_model_->OnNavigationEvent(
      kTabID1, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, GURL(),
      std::string(), nullptr);
  EXPECT_FALSE(IsTrackingDataUse(kTabID1));

  EXPECT_FALSE(data_use_tab_model_->WouldNavigationEventEndTracking(
      kTabID1, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, GURL(), nullptr));

  ExpectTabEntrySize(TabEntrySize::ZERO);
}

// Tests that Exit navigation event ends the tracking.
TEST_F(DataUseTabModelTest, NavigationEnterAndExitEvent) {
  std::vector<std::string> app_package_names, domain_regexes, labels;

  app_package_names.push_back(std::string());
  domain_regexes.push_back(kURLFoo);
  labels.push_back(kTestLabel2);

  RegisterURLRegexes(app_package_names, domain_regexes, labels);

  auto navigation_entry = CreateNavigationEntry(kURLFoo);
  EXPECT_FALSE(data_use_tab_model_->WouldNavigationEventEndTracking(
      kTabID1, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, GURL(kURLFoo),
      navigation_entry.get()));
  data_use_tab_model_->OnNavigationEvent(
      kTabID1, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, GURL(kURLFoo),
      std::string(), navigation_entry.get());
  ExpectDataUseLabelInNavigationEntry(*navigation_entry, kTestLabel2);
  ExpectTabEntrySize(TabEntrySize::ONE);
  EXPECT_TRUE(IsTrackingDataUse(kTabID1));
  EXPECT_TRUE(data_use_tab_model_->WouldNavigationEventEndTracking(
      kTabID1, DataUseTabModel::TRANSITION_BOOKMARK, GURL(), nullptr));
}

// Tests that any of the Enter transition events start the tracking.
TEST_F(DataUseTabModelTest, AllNavigationEnterEvents) {
  const struct {
    DataUseTabModel::TransitionType transition;
    std::string url;
    std::string package;
    std::string expect_label;
  } all_enter_transition_tests[] = {
      {DataUseTabModel::TRANSITION_CUSTOM_TAB, std::string(), kPackageFoo,
       kTestLabel1},
      {DataUseTabModel::TRANSITION_CUSTOM_TAB, std::string(), kPackageBar,
       kTestLabel3},
      {DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, kURLFoo, std::string(),
       kTestLabel2},
      {DataUseTabModel::TRANSITION_OMNIBOX_NAVIGATION, kURLFoo, std::string(),
       kTestLabel2},
      {DataUseTabModel::TRANSITION_LINK, kURLFoo, std::string(), kTestLabel2},
      {DataUseTabModel::TRANSITION_RELOAD, kURLFoo, std::string(), kTestLabel2},
  };
  std::vector<std::string> app_package_names, domain_regexes, labels;

  app_package_names.push_back(kPackageFoo);
  domain_regexes.push_back(std::string());
  labels.push_back(kTestLabel1);
  app_package_names.push_back(std::string());
  domain_regexes.push_back(kURLFoo);
  labels.push_back(kTestLabel2);
  app_package_names.push_back(kPackageBar);
  domain_regexes.push_back(std::string());
  labels.push_back(kTestLabel3);

  RegisterURLRegexes(app_package_names, domain_regexes, labels);

  int num_tabs = 0;
  for (const auto& test : all_enter_transition_tests) {
    SessionID tab_id = SessionID::NewUnique();
    EXPECT_FALSE(IsTrackingDataUse(tab_id));
    ExpectEmptyTrackingInfo(tab_id);

    // Tracking should start.
    auto navigation_entry = CreateNavigationEntry(test.url);
    data_use_tab_model_->OnNavigationEvent(tab_id, test.transition,
                                           GURL(test.url), test.package,
                                           navigation_entry.get());

    const std::string expected_tag =
        test.transition == DataUseTabModel::TRANSITION_CUSTOM_TAB
            ? DataUseTabModel::kCustomTabTag
            : DataUseTabModel::kDefaultTag;

    EXPECT_TRUE(IsTrackingDataUse(tab_id));
    ExpectTrackingInfo(tab_id, test.expect_label, expected_tag);
    if (test.transition != DataUseTabModel::TRANSITION_CUSTOM_TAB) {
      ExpectDataUseLabelInNavigationEntry(*navigation_entry, test.expect_label);

      auto navigation_entry_bar = CreateNavigationEntry(kURLBar);
      EXPECT_TRUE(data_use_tab_model_->WouldNavigationEventEndTracking(
          tab_id, DataUseTabModel::TRANSITION_OMNIBOX_NAVIGATION, GURL(kURLBar),
          navigation_entry_bar.get()));
    }
    ExpectTabEntrySize(++num_tabs);
    EXPECT_EQ(test.transition == DataUseTabModel::TRANSITION_CUSTOM_TAB,
              IsCustomTabPackageMatch(tab_id));
  }
}

// Tests that any of the Exit transition events end the tracking.
TEST_F(DataUseTabModelTest, AllNavigationExitEvents) {
  const struct {
    DataUseTabModel::TransitionType transition;
    std::string url;
  } all_exit_transition_tests[] = {
      {DataUseTabModel::TRANSITION_BOOKMARK, std::string()},
      {DataUseTabModel::TRANSITION_HISTORY_ITEM, std::string()},
      {DataUseTabModel::TRANSITION_OMNIBOX_NAVIGATION, kURLFooBar},
      {DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, kURLFooBar},
  };
  std::vector<std::string> app_package_names, domain_regexes, labels;

  app_package_names.push_back(std::string());
  domain_regexes.push_back(kURLFoo);
  labels.push_back(kTestLabel1);

  RegisterURLRegexes(app_package_names, domain_regexes, labels);

  int num_tabs = 0;
  for (const auto& test : all_exit_transition_tests) {
    SessionID tab_id = SessionID::NewUnique();
    EXPECT_FALSE(IsTrackingDataUse(tab_id));
    auto navigation_entry = CreateNavigationEntry(kURLFoo);
    data_use_tab_model_->OnNavigationEvent(
        tab_id, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, GURL(kURLFoo),
        std::string(), navigation_entry.get());
    EXPECT_TRUE(IsTrackingDataUse(tab_id));
    ExpectDataUseLabelInNavigationEntry(*navigation_entry, kTestLabel1);

    // Tracking should end.
    navigation_entry = CreateNavigationEntry(test.url);
    EXPECT_TRUE(data_use_tab_model_->WouldNavigationEventEndTracking(
        tab_id, test.transition, GURL(test.url), navigation_entry.get()));
    data_use_tab_model_->OnNavigationEvent(tab_id, test.transition,
                                           GURL(test.url), std::string(),
                                           navigation_entry.get());
    EXPECT_FALSE(data_use_tab_model_->WouldNavigationEventEndTracking(
        tab_id, test.transition, GURL(test.url), navigation_entry.get()));
    ExpectDataUseLabelInNavigationEntry(*navigation_entry, std::string());

    EXPECT_FALSE(IsTrackingDataUse(tab_id));
    ExpectTabEntrySize(++num_tabs);
  }
}

// Tests that transition events that can be enter or exit, are able to start and
// end the tracking with correct labels.
TEST_F(DataUseTabModelTest, AllNavigationExitAndEnterEvents) {
  const DataUseTabModel::TransitionType all_test_transitions[] = {
      DataUseTabModel::TRANSITION_OMNIBOX_SEARCH,
      DataUseTabModel::TRANSITION_OMNIBOX_NAVIGATION};
  std::vector<std::string> app_package_names, domain_regexes, labels;

  app_package_names.push_back(std::string());
  domain_regexes.push_back(kURLFoo);
  labels.push_back(kTestLabel1);

  RegisterURLRegexes(app_package_names, domain_regexes, labels);

  int num_tabs = 0;
  for (const auto& transition : all_test_transitions) {
    SessionID tab_id = SessionID::NewUnique();
    EXPECT_FALSE(IsTrackingDataUse(tab_id));
    auto navigation_entry = CreateNavigationEntry(kURLFoo);
    data_use_tab_model_->OnNavigationEvent(tab_id, transition, GURL(kURLFoo),
                                           std::string(),
                                           navigation_entry.get());
    EXPECT_TRUE(IsTrackingDataUse(tab_id));
    ExpectTrackingInfo(tab_id, kTestLabel1, DataUseTabModel::kDefaultTag);
    ExpectDataUseLabelInNavigationEntry(*navigation_entry, kTestLabel1);

    navigation_entry = CreateNavigationEntry(kURLBar);
    EXPECT_TRUE(data_use_tab_model_->WouldNavigationEventEndTracking(
        tab_id, DataUseTabModel::TRANSITION_OMNIBOX_NAVIGATION, GURL(kURLBar),
        navigation_entry.get()));

    // No change in label.
    navigation_entry = CreateNavigationEntry(kURLFoo);
    data_use_tab_model_->OnNavigationEvent(tab_id, transition, GURL(kURLFoo),
                                           std::string(),
                                           navigation_entry.get());
    EXPECT_TRUE(IsTrackingDataUse(tab_id));
    ExpectTrackingInfo(tab_id, kTestLabel1, DataUseTabModel::kDefaultTag);
    ExpectDataUseLabelInNavigationEntry(*navigation_entry, kTestLabel1);

    // Tracking should end.
    navigation_entry = CreateNavigationEntry(std::string());
    EXPECT_TRUE(data_use_tab_model_->WouldNavigationEventEndTracking(
        tab_id, transition, GURL(), navigation_entry.get()));
    data_use_tab_model_->OnNavigationEvent(
        tab_id, transition, GURL(), std::string(), navigation_entry.get());

    EXPECT_FALSE(IsTrackingDataUse(tab_id));
    ExpectTabEntrySize(++num_tabs);
  }
}

// Tests that back-forward navigations start or end tracking depending on the
// tracking state of the initial navigation.
// TODO(rajendrant) Add an integration test with TestWebContents that calls
// NavigateAndCommit(), test if the navigation_entry is popped up correctly,
// and label is applied from there.
TEST_F(DataUseTabModelTest, BackForwardNavigationSequence) {
  std::vector<std::string> app_package_names, domain_regexes, labels;
  MockTabDataUseObserver mock_observer;

  app_package_names.push_back(kPackageFoo);
  domain_regexes.push_back(kURLFoo);
  labels.push_back(kTestLabel1);
  RegisterURLRegexes(app_package_names, domain_regexes, labels);

  data_use_tab_model_->AddObserver(&mock_observer);

  // Start tracking.
  EXPECT_CALL(mock_observer, NotifyTrackingStarting(kTabID1)).Times(1);
  auto navigation_entry_foo = CreateNavigationEntry(kURLFoo);
  data_use_tab_model_->OnNavigationEvent(
      kTabID1, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, GURL(kURLFoo),
      std::string(), navigation_entry_foo.get());

  EXPECT_TRUE(IsTrackingDataUse(kTabID1));
  testing::Mock::VerifyAndClearExpectations(&mock_observer);
  ExpectTrackingInfo(kTabID1, kTestLabel1, DataUseTabModel::kDefaultTag);
  ExpectDataUseLabelInNavigationEntry(*navigation_entry_foo, kTestLabel1);

  // Navigate to some URL which does not match regex. Still continues tracking.
  auto navigation_entry_bar = CreateNavigationEntry(kURLBar);
  data_use_tab_model_->OnNavigationEvent(
      kTabID1, DataUseTabModel::TRANSITION_LINK, GURL(kURLBar), std::string(),
      navigation_entry_bar.get());
  ExpectTrackingInfo(kTabID1, kTestLabel1, DataUseTabModel::kDefaultTag);
  ExpectDataUseLabelInNavigationEntry(*navigation_entry_foo, kTestLabel1);

  // End tracking.
  auto navigation_entry_baz = CreateNavigationEntry(kURLBaz);
  EXPECT_CALL(mock_observer, NotifyTrackingEnding(kTabID1)).Times(1);
  EXPECT_TRUE(data_use_tab_model_->WouldNavigationEventEndTracking(
      kTabID1, DataUseTabModel::TRANSITION_OMNIBOX_NAVIGATION, GURL(kURLBaz),
      navigation_entry_baz.get()));
  data_use_tab_model_->OnNavigationEvent(
      kTabID1, DataUseTabModel::TRANSITION_OMNIBOX_NAVIGATION, GURL(kURLBaz),
      std::string(), navigation_entry_baz.get());
  EXPECT_FALSE(IsTrackingDataUse(kTabID1));
  testing::Mock::VerifyAndClearExpectations(&mock_observer);
  ExpectDataUseLabelInNavigationEntry(*navigation_entry_baz, std::string());

  // Navigating backward starts tracking.
  EXPECT_CALL(mock_observer, NotifyTrackingStarting(kTabID1)).Times(1);
  data_use_tab_model_->OnNavigationEvent(
      kTabID1, DataUseTabModel::TRANSITION_FORWARD_BACK, GURL(kURLBar),
      std::string(), navigation_entry_bar.get());
  EXPECT_TRUE(IsTrackingDataUse(kTabID1));
  testing::Mock::VerifyAndClearExpectations(&mock_observer);
  ExpectTrackingInfo(kTabID1, kTestLabel1, DataUseTabModel::kDefaultTag);
  ExpectDataUseLabelInNavigationEntry(*navigation_entry_bar, kTestLabel1);

  // Continue navigating backwards.
  data_use_tab_model_->OnNavigationEvent(
      kTabID1, DataUseTabModel::TRANSITION_FORWARD_BACK, GURL(kURLFoo),
      std::string(), navigation_entry_foo.get());
  EXPECT_TRUE(IsTrackingDataUse(kTabID1));
  testing::Mock::VerifyAndClearExpectations(&mock_observer);
  ExpectTrackingInfo(kTabID1, kTestLabel1, DataUseTabModel::kDefaultTag);
  ExpectDataUseLabelInNavigationEntry(*navigation_entry_foo, kTestLabel1);

  // Navigating forward continues tracking.
  data_use_tab_model_->OnNavigationEvent(
      kTabID1, DataUseTabModel::TRANSITION_FORWARD_BACK, GURL(kURLBar),
      std::string(), navigation_entry_bar.get());
  EXPECT_TRUE(IsTrackingDataUse(kTabID1));
  ExpectTrackingInfo(kTabID1, kTestLabel1, DataUseTabModel::kDefaultTag);
  ExpectDataUseLabelInNavigationEntry(*navigation_entry_bar, kTestLabel1);

  // Continue navigating forwards ends tracking.
  EXPECT_CALL(mock_observer, NotifyTrackingEnding(kTabID1)).Times(1);
  EXPECT_TRUE(data_use_tab_model_->WouldNavigationEventEndTracking(
      kTabID1, DataUseTabModel::TRANSITION_FORWARD_BACK, GURL(kURLBaz),
      navigation_entry_baz.get()));
  data_use_tab_model_->OnNavigationEvent(
      kTabID1, DataUseTabModel::TRANSITION_FORWARD_BACK, GURL(kURLBaz),
      std::string(), navigation_entry_baz.get());
  EXPECT_FALSE(IsTrackingDataUse(kTabID1));
  ExpectDataUseLabelInNavigationEntry(*navigation_entry_baz, std::string());
}

// Tests that a sequence of transitions simulating user actions are able to
// start and end the tracking with correct label.
TEST_F(DataUseTabModelTest, SingleTabTransitionSequence) {
  std::vector<std::string> app_package_names, domain_regexes, labels;
  MockTabDataUseObserver mock_observer;

  const struct {
    DataUseTabModel::TransitionType transition;
    std::string url;
    std::string package;
    std::string expected_label;
    TrackingState observer_event;
  } transition_tests[] = {
      // Opening matching URL from omnibox starts tracking.
      {DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, kURLBaz, std::string(),
       kTestLabel1, STARTED},
      // Clicking on links in the page continues tracking.
      {DataUseTabModel::TRANSITION_LINK, kURLBar, std::string(), kTestLabel1,
       CONTINUES},
      {DataUseTabModel::TRANSITION_LINK, kURLFooBar, std::string(), kTestLabel1,
       CONTINUES},
      // Navigating to a non matching URL from omnibox ends tracking.
      {DataUseTabModel::TRANSITION_OMNIBOX_NAVIGATION, kURLBar, std::string(),
       std::string(), ENDED},
      // Clicking on non matching URL links in the page does not start tracking.
      {DataUseTabModel::TRANSITION_LINK, kURLFooBar, std::string(),
       std::string(), NONE},
      // Clicking on matching URL links in the page starts tracking.
      {DataUseTabModel::TRANSITION_LINK, kURLFoo, std::string(), kTestLabel2,
       STARTED},
      // Navigating to bookmark with matching URL does not end tracking.
      {DataUseTabModel::TRANSITION_BOOKMARK, kURLFoo, std::string(),
       kTestLabel2, CONTINUES},
      // Navigating to bookmark with non-matching URL ends tracking.
      {DataUseTabModel::TRANSITION_BOOKMARK, std::string(), std::string(),
       std::string(), ENDED},
      // Navigating to a matching URL from omnibox starts tracking.
      {DataUseTabModel::TRANSITION_OMNIBOX_NAVIGATION, kURLFoo, std::string(),
       kTestLabel2, STARTED},
      // Navigating to history item ends tracking.
      {DataUseTabModel::TRANSITION_HISTORY_ITEM, std::string(), std::string(),
       std::string(), ENDED},
  };

  app_package_names.push_back(std::string());
  domain_regexes.push_back(kURLBaz);
  labels.push_back(kTestLabel1);
  app_package_names.push_back(std::string());
  domain_regexes.push_back(kURLFoo);
  labels.push_back(kTestLabel2);
  RegisterURLRegexes(app_package_names, domain_regexes, labels);

  data_use_tab_model_->AddObserver(&mock_observer);
  for (auto const& test : transition_tests) {
    tick_clock_.Advance(base::TimeDelta::FromSeconds(1));

    auto navigation_entry = CreateNavigationEntry(test.url);

    if (test.observer_event == ENDED) {
      EXPECT_TRUE(data_use_tab_model_->WouldNavigationEventEndTracking(
          kTabID1, test.transition, GURL(test.url), navigation_entry.get()));
    }

    EXPECT_CALL(mock_observer, NotifyTrackingStarting(kTabID1))
        .Times(test.observer_event == STARTED ? 1 : 0);
    EXPECT_CALL(mock_observer, NotifyTrackingEnding(kTabID1))
        .Times(test.observer_event == ENDED ? 1 : 0);

    data_use_tab_model_->OnNavigationEvent(kTabID1, test.transition,
                                           GURL(test.url), test.package,
                                           navigation_entry.get());
    tick_clock_.Advance(base::TimeDelta::FromSeconds(1));

    EXPECT_EQ(!test.expected_label.empty(), IsTrackingDataUse(kTabID1));
    ExpectTrackingInfo(kTabID1, test.expected_label,
                       DataUseTabModel::kDefaultTag);
    ExpectDataUseLabelInNavigationEntry(*navigation_entry, test.expected_label);

    if (test.observer_event == STARTED || test.observer_event == CONTINUES) {
      EXPECT_TRUE(data_use_tab_model_->WouldNavigationEventEndTracking(
          kTabID1, DataUseTabModel::TRANSITION_BOOKMARK, GURL(), nullptr));
    }

    testing::Mock::VerifyAndClearExpectations(&mock_observer);
  }
}

// Tests that a sequence of transitions in a custom tab that has an active
// tracking session never ends the tracking.
TEST_F(DataUseTabModelTest, SingleCustomTabTransitionSequence) {
  std::vector<std::string> app_package_names, domain_regexes, labels;
  MockTabDataUseObserver mock_observer;

  const struct {
    DataUseTabModel::TransitionType transition;
    std::string url;
    std::string package;
    std::string expected_label;
    TrackingState observer_event;
  } transition_tests[] = {
      // Opening Custom Tab with matching package starts tracking.
      {DataUseTabModel::TRANSITION_CUSTOM_TAB, std::string(), kPackageFoo,
       kTestLabel1, STARTED},
      // Clicking on links in the page continues tracking.
      {DataUseTabModel::TRANSITION_LINK, kURLBar, std::string(), kTestLabel1,
       CONTINUES},
      {DataUseTabModel::TRANSITION_LINK, kURLFooBar, std::string(), kTestLabel1,
       CONTINUES},
      // Clicking on bookmark continues tracking.
      {DataUseTabModel::TRANSITION_BOOKMARK, kURLFooBar, std::string(),
       kTestLabel1, CONTINUES},
      // Reloading the page continues tracking.
      {DataUseTabModel::TRANSITION_RELOAD, kURLFooBar, std::string(),
       kTestLabel1, CONTINUES},
      // Clicking on links in the page continues tracking.
      {DataUseTabModel::TRANSITION_LINK, kURLBar, std::string(), kTestLabel1,
       CONTINUES},
  };

  app_package_names.push_back(kPackageFoo);
  domain_regexes.push_back(std::string());
  labels.push_back(kTestLabel1);
  app_package_names.push_back(std::string());
  domain_regexes.push_back(kURLFoo);
  labels.push_back(kTestLabel2);
  RegisterURLRegexes(app_package_names, domain_regexes, labels);

  data_use_tab_model_->AddObserver(&mock_observer);
  for (auto const& test : transition_tests) {
    tick_clock_.Advance(base::TimeDelta::FromSeconds(1));

    EXPECT_CALL(mock_observer, NotifyTrackingStarting(kTabID1))
        .Times(test.observer_event == STARTED ? 1 : 0);
    EXPECT_CALL(mock_observer, NotifyTrackingEnding(kTabID1))
        .Times(test.observer_event == ENDED ? 1 : 0);

    auto navigation_entry = CreateNavigationEntry(test.url);
    data_use_tab_model_->OnNavigationEvent(kTabID1, test.transition,
                                           GURL(test.url), test.package,
                                           navigation_entry.get());
    tick_clock_.Advance(base::TimeDelta::FromSeconds(1));

    EXPECT_EQ(!test.expected_label.empty(), IsTrackingDataUse(kTabID1));
    ExpectTrackingInfo(kTabID1, test.expected_label,
                       DataUseTabModel::kCustomTabTag);
    ExpectDataUseLabelInNavigationEntry(*navigation_entry, test.expected_label);

    // Tracking never ends.
    EXPECT_FALSE(data_use_tab_model_->WouldNavigationEventEndTracking(
        kTabID1, DataUseTabModel::TRANSITION_LINK, GURL(), nullptr));

    testing::Mock::VerifyAndClearExpectations(&mock_observer);
  }
}

// Tests that tab model is notified when tracking labels are removed.
TEST_F(DataUseTabModelTest, LabelRemoved) {
  std::vector<std::string> labels;

  tick_clock_.Advance(base::TimeDelta::FromSeconds(1));
  labels.push_back(kTestLabel1);
  labels.push_back(kTestLabel2);
  labels.push_back(kTestLabel3);
  RegisterURLRegexes(std::vector<std::string>(labels.size(), std::string()),
                     std::vector<std::string>(labels.size(), kURLFoo), labels);

  auto navigation_entry = CreateNavigationEntry(kURLFoo);
  data_use_tab_model_->OnNavigationEvent(
      kTabID1, DataUseTabModel::TRANSITION_OMNIBOX_SEARCH, GURL(kURLFoo),
      std::string(), navigation_entry.get());
  EXPECT_TRUE(IsTrackingDataUse(kTabID1));

  labels.clear();
  labels.push_back(kTestLabel2);
  labels.push_back("label_4");
  labels.push_back("label_5");
  RegisterURLRegexes(std::vector<std::string>(labels.size(), std::string()),
                     std::vector<std::string>(labels.size(), kURLFoo), labels);
  EXPECT_FALSE(IsTrackingDataUse(kTabID1));
}

// Tests the behavior when the external control app is uninstalled. When the app
// gets uninstalled the active tracking sessions should end and the existing
// matching rules should be cleared.
TEST_F(DataUseTabModelTest, MatchingRuleClearedOnControlAppUninstall) {
  std::vector<std::string> app_package_names, domain_regexes, labels;

  app_package_names.push_back(kPackageFoo);
  domain_regexes.push_back(kURLFoo);
  labels.push_back(kTestLabel1);

  RegisterURLRegexes(app_package_names, domain_regexes, labels);

  StartTrackingDataUse(kTabID1, kTestLabel1);
  EXPECT_TRUE(IsTrackingDataUse(kTabID1));
  EXPECT_TRUE(data_use_tab_model_->data_use_matcher_->HasRules());

  data_use_tab_model_->OnControlAppInstallStateChange(false);

  EXPECT_FALSE(IsTrackingDataUse(kTabID1));
  EXPECT_FALSE(data_use_tab_model_->data_use_matcher_->HasRules());
}

// Tests that |OnDataUseTabModelReady| is sent to observers when the external
// control app not installed callback was received.
TEST_F(DataUseTabModelTest, ReadyForNavigationEventWhenControlAppNotInstalled) {
  MockTabDataUseObserver mock_observer;
  data_use_tab_model_->AddObserver(&mock_observer);

  EXPECT_FALSE(data_use_tab_model_->is_ready_for_navigation_event());
  EXPECT_CALL(mock_observer, OnDataUseTabModelReady()).Times(1);

  data_use_tab_model_->OnControlAppInstallStateChange(false);
  EXPECT_TRUE(data_use_tab_model_->is_ready_for_navigation_event());
  testing::Mock::VerifyAndClearExpectations(&mock_observer);

  // Subsequent install and uninstall of the control app does not trigger the
  // event.
  EXPECT_CALL(mock_observer, OnDataUseTabModelReady()).Times(0);
  data_use_tab_model_->OnControlAppInstallStateChange(true);
  data_use_tab_model_->OnControlAppInstallStateChange(false);
  testing::Mock::VerifyAndClearExpectations(&mock_observer);
  EXPECT_TRUE(data_use_tab_model_->is_ready_for_navigation_event());
}

// Tests that |OnDataUseTabModelReady| is sent to observers when the first rule
// fetch is complete.
TEST_F(DataUseTabModelTest, ReadyForNavigationEventAfterRuleFetch) {
  MockTabDataUseObserver mock_observer;
  std::vector<std::string> app_package_names, domain_regexes, labels;

  app_package_names.push_back(kPackageFoo);
  domain_regexes.push_back(kURLFoo);
  labels.push_back(kTestLabel1);
  data_use_tab_model_->AddObserver(&mock_observer);

  EXPECT_FALSE(data_use_tab_model_->is_ready_for_navigation_event());
  EXPECT_CALL(mock_observer, OnDataUseTabModelReady()).Times(1);

  // First rule fetch triggers the event.
  RegisterURLRegexes(app_package_names, domain_regexes, labels);
  testing::Mock::VerifyAndClearExpectations(&mock_observer);

  // Subsequent rule fetches, uninstall and install of the control app does not
  // trigger the event.
  EXPECT_CALL(mock_observer, OnDataUseTabModelReady()).Times(0);
  RegisterURLRegexes(app_package_names, domain_regexes, labels);
  data_use_tab_model_->OnControlAppInstallStateChange(false);
  data_use_tab_model_->OnControlAppInstallStateChange(true);
  testing::Mock::VerifyAndClearExpectations(&mock_observer);
  EXPECT_TRUE(data_use_tab_model_->is_ready_for_navigation_event());
}

}  // namespace android

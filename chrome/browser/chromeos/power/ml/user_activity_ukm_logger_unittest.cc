// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_ukm_logger_impl.h"

#include <memory>
#include <vector>

#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "chrome/browser/chromeos/power/ml/user_activity_manager.h"
#include "components/ukm/test_ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace power {
namespace ml {

using ukm::builders::UserActivity;
using ukm::builders::UserActivityId;

class UserActivityUkmLoggerTest : public testing::Test {
 public:
  UserActivityUkmLoggerTest() {
    // These values are arbitrary but must correspond with the values
    // in CheckUserActivityValues.
    UserActivityEvent::Event* event = user_activity_event_.mutable_event();
    event->set_log_duration_sec(395);
    event->set_reason(UserActivityEvent::Event::USER_ACTIVITY);
    event->set_type(UserActivityEvent::Event::REACTIVATE);
    event->set_screen_dim_occurred(true);
    event->set_screen_off_occurred(true);
    event->set_screen_lock_occurred(true);

    // In the order of metrics names in ukm.
    UserActivityEvent::Features* features =
        user_activity_event_.mutable_features();
    features->set_battery_percent(96.0);
    features->set_device_management(UserActivityEvent::Features::UNMANAGED);
    features->set_device_mode(UserActivityEvent::Features::CLAMSHELL);
    features->set_device_type(UserActivityEvent::Features::CHROMEBOOK);
    features->set_last_activity_day(UserActivityEvent::Features::MON);
    features->set_last_activity_time_sec(7300);
    features->set_last_user_activity_time_sec(3800);
    features->set_key_events_in_last_hour(20000);
    features->set_recent_time_active_sec(10);
    features->set_video_playing_time_sec(800);
    features->set_on_to_dim_sec(100);
    features->set_dim_to_screen_off_sec(200);
    features->set_screen_dimmed_initially(false);
    features->set_screen_locked_initially(false);
    features->set_screen_off_initially(false);
    features->set_time_since_last_mouse_sec(100);
    features->set_time_since_last_touch_sec(311);
    features->set_time_since_video_ended_sec(400);
    features->set_mouse_events_in_last_hour(89);
    features->set_touch_events_in_last_hour(1890);

    user_activity_logger_delegate_ukm_.ukm_recorder_ = &recorder_;
  }

 protected:
  void LogActivity(const UserActivityEvent& event,
                   const std::map<ukm::SourceId, TabProperty>& open_tabs) {
    user_activity_logger_delegate_ukm_.LogActivity(event, open_tabs);
  }

  void CheckUserActivityValues(const ukm::mojom::UkmEntry* entry) {
    recorder_.ExpectEntryMetric(entry, UserActivity::kEventLogDurationName,
                                395);
    recorder_.ExpectEntryMetric(entry, UserActivity::kEventReasonName,
                                UserActivityEvent::Event::USER_ACTIVITY);
    recorder_.ExpectEntryMetric(entry, UserActivity::kEventTypeName,
                                UserActivityEvent::Event::REACTIVATE);
    recorder_.ExpectEntryMetric(entry, UserActivity::kBatteryPercentName, 95);
    recorder_.ExpectEntryMetric(entry, UserActivity::kDeviceManagementName,
                                UserActivityEvent::Features::UNMANAGED);
    recorder_.ExpectEntryMetric(entry, UserActivity::kDeviceModeName,
                                UserActivityEvent::Features::CLAMSHELL);
    recorder_.ExpectEntryMetric(entry, UserActivity::kDeviceTypeName,
                                UserActivityEvent::Features::CHROMEBOOK);
    recorder_.ExpectEntryMetric(entry, UserActivity::kLastActivityDayName,
                                UserActivityEvent::Features::MON);
    recorder_.ExpectEntryMetric(entry, UserActivity::kKeyEventsInLastHourName,
                                10000);
    recorder_.ExpectEntryMetric(entry, UserActivity::kLastActivityTimeName, 2);
    recorder_.ExpectEntryMetric(entry, UserActivity::kLastUserActivityTimeName,
                                1);
    recorder_.ExpectEntryMetric(entry, UserActivity::kMouseEventsInLastHourName,
                                89);
    EXPECT_FALSE(recorder_.EntryHasMetric(entry, UserActivity::kOnBatteryName));
    recorder_.ExpectEntryMetric(entry, UserActivity::kRecentTimeActiveName, 10);
    recorder_.ExpectEntryMetric(entry,
                                UserActivity::kRecentVideoPlayingTimeName, 600);
    recorder_.ExpectEntryMetric(entry, UserActivity::kScreenDimDelayName, 100);
    recorder_.ExpectEntryMetric(entry, UserActivity::kScreenDimmedInitiallyName,
                                false);
    recorder_.ExpectEntryMetric(entry, UserActivity::kScreenDimOccurredName,
                                true);
    recorder_.ExpectEntryMetric(entry, UserActivity::kScreenDimToOffDelayName,
                                200);
    recorder_.ExpectEntryMetric(entry, UserActivity::kScreenLockedInitiallyName,
                                false);
    recorder_.ExpectEntryMetric(entry, UserActivity::kScreenLockOccurredName,
                                true);
    recorder_.ExpectEntryMetric(entry, UserActivity::kScreenOffInitiallyName,
                                false);
    recorder_.ExpectEntryMetric(entry, UserActivity::kScreenOffOccurredName,
                                true);

    recorder_.ExpectEntryMetric(entry, UserActivity::kSequenceIdName, 1);
    EXPECT_FALSE(
        recorder_.EntryHasMetric(entry, UserActivity::kTimeSinceLastKeyName));
    recorder_.ExpectEntryMetric(entry, UserActivity::kTimeSinceLastMouseName,
                                100);
    recorder_.ExpectEntryMetric(entry, UserActivity::kTimeSinceLastTouchName,
                                311);
    recorder_.ExpectEntryMetric(
        entry, UserActivity::kTimeSinceLastVideoEndedName, 360);
    recorder_.ExpectEntryMetric(entry, UserActivity::kTouchEventsInLastHourName,
                                1000);
  }

  UserActivityEvent user_activity_event_;
  ukm::TestUkmRecorder recorder_;

 private:
  UserActivityUkmLoggerImpl user_activity_logger_delegate_ukm_;
  DISALLOW_COPY_AND_ASSIGN(UserActivityUkmLoggerTest);
};

TEST_F(UserActivityUkmLoggerTest, BucketEveryFivePercents) {
  const std::vector<int> original_values = {0, 14, 15, 100};
  const std::vector<int> results = {0, 10, 15, 100};
  constexpr UserActivityUkmLoggerImpl::Bucket buckets[] = {{100, 5}};

  for (size_t i = 0; i < original_values.size(); ++i) {
    EXPECT_EQ(results[i], UserActivityUkmLoggerImpl::Bucketize(
                              original_values[i], buckets, arraysize(buckets)));
  }
}

TEST_F(UserActivityUkmLoggerTest, Bucketize) {
  const std::vector<int> original_values = {0,   18,  59,  60,  62,  69,  72,
                                            299, 300, 306, 316, 599, 600, 602};
  constexpr UserActivityUkmLoggerImpl::Bucket buckets[] = {
      {60, 1}, {300, 10}, {600, 20}};
  const std::vector<int> results = {0,   18,  59,  60,  60,  60,  70,
                                    290, 300, 300, 300, 580, 600, 600};
  for (size_t i = 0; i < original_values.size(); ++i) {
    EXPECT_EQ(results[i], UserActivityUkmLoggerImpl::Bucketize(
                              original_values[i], buckets, arraysize(buckets)));
  }
}

TEST_F(UserActivityUkmLoggerTest, BasicLogging) {
  TabProperty properties[2];

  properties[0].is_active = true;
  properties[0].is_browser_focused = false;
  properties[0].is_browser_visible = true;
  properties[0].is_topmost_browser = false;
  properties[0].engagement_score = 90;
  properties[0].content_type =
      metrics::TabMetricsEvent::CONTENT_TYPE_APPLICATION;
  properties[0].has_form_entry = false;

  properties[1].is_active = false;
  properties[1].is_browser_focused = true;
  properties[1].is_browser_visible = false;
  properties[1].is_topmost_browser = true;
  properties[1].engagement_score = 0;
  properties[1].content_type = metrics::TabMetricsEvent::CONTENT_TYPE_TEXT_HTML;
  properties[1].has_form_entry = true;

  ukm::SourceId source_ids[2];
  for (int i = 0; i < 2; ++i)
    source_ids[i] = recorder_.GetNewSourceID();
  std::map<ukm::SourceId, TabProperty> source_properties(
      {{source_ids[0], properties[0]}, {source_ids[1], properties[1]}});
  LogActivity(user_activity_event_, source_properties);

  const auto& activity_entries =
      recorder_.GetEntriesByName(UserActivity::kEntryName);
  EXPECT_EQ(1u, activity_entries.size());
  const ukm::mojom::UkmEntry* activity_entry = activity_entries[0];
  CheckUserActivityValues(activity_entry);

  const ukm::SourceId kSourceId = activity_entry->source_id;
  const auto& activity_id_entries =
      recorder_.GetEntriesByName(UserActivityId::kEntryName);
  EXPECT_EQ(2u, activity_id_entries.size());

  const ukm::mojom::UkmEntry* entry0 = activity_id_entries[0];
  recorder_.ExpectEntryMetric(entry0, UserActivityId::kActivityIdName,
                              kSourceId);
  recorder_.ExpectEntryMetric(entry0, UserActivityId::kIsActiveName, 1);
  recorder_.ExpectEntryMetric(entry0, UserActivityId::kIsBrowserFocusedName, 0);
  recorder_.ExpectEntryMetric(entry0, UserActivityId::kIsBrowserVisibleName, 1);
  recorder_.ExpectEntryMetric(entry0, UserActivityId::kIsTopmostBrowserName, 0);
  recorder_.ExpectEntryMetric(entry0, UserActivityId::kSiteEngagementScoreName,
                              90);
  recorder_.ExpectEntryMetric(
      entry0, UserActivityId::kContentTypeName,
      metrics::TabMetricsEvent::CONTENT_TYPE_APPLICATION);
  recorder_.ExpectEntryMetric(entry0, UserActivityId::kHasFormEntryName, 0);

  const ukm::mojom::UkmEntry* entry1 = activity_id_entries[1];
  recorder_.ExpectEntryMetric(entry1, UserActivityId::kActivityIdName,
                              kSourceId);
  recorder_.ExpectEntryMetric(entry1, UserActivityId::kIsActiveName, 0);
  recorder_.ExpectEntryMetric(entry1, UserActivityId::kIsBrowserFocusedName, 1);
  recorder_.ExpectEntryMetric(entry1, UserActivityId::kIsBrowserVisibleName, 0);
  recorder_.ExpectEntryMetric(entry1, UserActivityId::kIsTopmostBrowserName, 1);
  recorder_.ExpectEntryMetric(entry1, UserActivityId::kSiteEngagementScoreName,
                              0);
  recorder_.ExpectEntryMetric(entry1, UserActivityId::kContentTypeName,
                              metrics::TabMetricsEvent::CONTENT_TYPE_TEXT_HTML);
  recorder_.ExpectEntryMetric(entry1, UserActivityId::kHasFormEntryName, 1);
}

// Tests what would be logged in Incognito: when source IDs are not provided.
TEST_F(UserActivityUkmLoggerTest, EmptySources) {
  std::map<ukm::SourceId, TabProperty> empty_source_properties;
  LogActivity(user_activity_event_, empty_source_properties);

  const auto& activity_entries =
      recorder_.GetEntriesByName(UserActivity::kEntryName);
  EXPECT_EQ(1u, activity_entries.size());
  const ukm::mojom::UkmEntry* activity_entry = activity_entries[0];

  CheckUserActivityValues(activity_entry);

  EXPECT_EQ(0u, recorder_.GetEntriesByName(UserActivityId::kEntryName).size());
}

TEST_F(UserActivityUkmLoggerTest, TwoUserActivityEvents) {
  // A second event will be logged. Values correspond with the checks below.
  UserActivityEvent user_activity_event2;
  UserActivityEvent::Event* event = user_activity_event2.mutable_event();
  event->set_log_duration_sec(35);
  event->set_reason(UserActivityEvent::Event::IDLE_SLEEP);
  event->set_type(UserActivityEvent::Event::TIMEOUT);

  UserActivityEvent::Features* features =
      user_activity_event2.mutable_features();
  features->set_battery_percent(86.0);
  features->set_device_management(UserActivityEvent::Features::MANAGED);
  features->set_device_mode(UserActivityEvent::Features::CLAMSHELL);
  features->set_device_type(UserActivityEvent::Features::CHROMEBOOK);
  features->set_last_activity_day(UserActivityEvent::Features::TUE);
  features->set_last_activity_time_sec(7300);
  features->set_last_user_activity_time_sec(3800);
  features->set_recent_time_active_sec(20);
  features->set_on_to_dim_sec(10);
  features->set_dim_to_screen_off_sec(20);
  features->set_time_since_last_mouse_sec(200);

  std::map<ukm::SourceId, TabProperty> empty_source_properties;
  LogActivity(user_activity_event_, empty_source_properties);
  LogActivity(user_activity_event2, empty_source_properties);

  const auto& activity_entries =
      recorder_.GetEntriesByName(UserActivity::kEntryName);
  EXPECT_EQ(2u, activity_entries.size());

  // Check the first user activity values.
  CheckUserActivityValues(activity_entries[0]);

  // Check the second user activity values.
  const ukm::mojom::UkmEntry* entry1 = activity_entries[1];
  recorder_.ExpectEntryMetric(entry1, UserActivity::kEventLogDurationName, 35);
  recorder_.ExpectEntryMetric(entry1, UserActivity::kEventReasonName,
                              UserActivityEvent::Event::IDLE_SLEEP);
  recorder_.ExpectEntryMetric(entry1, UserActivity::kEventTypeName,
                              UserActivityEvent::Event::TIMEOUT);
  recorder_.ExpectEntryMetric(entry1, UserActivity::kBatteryPercentName, 85);
  recorder_.ExpectEntryMetric(entry1, UserActivity::kDeviceManagementName,
                              UserActivityEvent::Features::MANAGED);
  recorder_.ExpectEntryMetric(entry1, UserActivity::kDeviceModeName,
                              UserActivityEvent::Features::CLAMSHELL);
  recorder_.ExpectEntryMetric(entry1, UserActivity::kDeviceTypeName,
                              UserActivityEvent::Features::CHROMEBOOK);
  recorder_.ExpectEntryMetric(entry1, UserActivity::kLastActivityDayName,
                              UserActivityEvent::Features::TUE);
  recorder_.ExpectEntryMetric(entry1, UserActivity::kLastActivityTimeName, 2);
  recorder_.ExpectEntryMetric(entry1, UserActivity::kLastUserActivityTimeName,
                              1);
  EXPECT_FALSE(recorder_.EntryHasMetric(entry1, UserActivity::kOnBatteryName));
  recorder_.ExpectEntryMetric(entry1, UserActivity::kRecentTimeActiveName, 20);
  recorder_.ExpectEntryMetric(entry1, UserActivity::kScreenDimDelayName, 10);
  recorder_.ExpectEntryMetric(entry1, UserActivity::kScreenDimToOffDelayName,
                              20);
  recorder_.ExpectEntryMetric(entry1, UserActivity::kSequenceIdName, 2);
  EXPECT_FALSE(
      recorder_.EntryHasMetric(entry1, UserActivity::kTimeSinceLastKeyName));
  recorder_.ExpectEntryMetric(entry1, UserActivity::kTimeSinceLastMouseName,
                              200);

  EXPECT_EQ(0u, recorder_.GetEntriesByName(UserActivityId::kEntryName).size());
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos

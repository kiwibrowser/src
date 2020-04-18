// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>

#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "chrome/browser/chromeos/power/ml/user_activity_manager.h"
#include "chrome/browser/chromeos/power/ml/user_activity_ukm_logger_impl.h"
#include "content/public/browser/web_contents.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "ui/gfx/native_widget_types.h"

namespace chromeos {
namespace power {
namespace ml {

namespace {

constexpr UserActivityUkmLoggerImpl::Bucket kBatteryPercentBuckets[] = {
    {100, 5}};

constexpr UserActivityUkmLoggerImpl::Bucket kEventLogDurationBuckets[] = {
    {600, 1},
    {1200, 10},
    {1800, 20}};

constexpr UserActivityUkmLoggerImpl::Bucket kUserInputEventBuckets[] = {
    {100, 1},
    {1000, 100},
    {10000, 1000}};

constexpr UserActivityUkmLoggerImpl::Bucket kRecentVideoPlayingTimeBuckets[] = {
    {60, 1},
    {1200, 300},
    {3600, 600},
    {18000, 1800}};

constexpr UserActivityUkmLoggerImpl::Bucket kTimeSinceLastVideoEndedBuckets[] =
    {{60, 1}, {600, 60}, {1200, 300}, {3600, 600}, {18000, 1800}};

}  // namespace

int UserActivityUkmLoggerImpl::Bucketize(int original_value,
                                         const Bucket* buckets,
                                         size_t num_buckets) {
  DCHECK_GE(original_value, 0);
  DCHECK(buckets);
  for (size_t i = 0; i < num_buckets; ++i) {
    const Bucket& bucket = buckets[i];
    if (original_value < bucket.boundary_end) {
      return bucket.rounding * (original_value / bucket.rounding);
    }
  }
  return buckets[num_buckets - 1].boundary_end;
}

UserActivityUkmLoggerImpl::UserActivityUkmLoggerImpl()
    : ukm_recorder_(ukm::UkmRecorder::Get()) {}

UserActivityUkmLoggerImpl::~UserActivityUkmLoggerImpl() = default;

void UserActivityUkmLoggerImpl::LogActivity(
    const UserActivityEvent& event,
    const std::map<ukm::SourceId, TabProperty>& open_tabs) {
  DCHECK(ukm_recorder_);
  ukm::SourceId source_id = ukm_recorder_->GetNewSourceID();
  ukm::builders::UserActivity user_activity(source_id);
  user_activity.SetSequenceId(next_sequence_id_++)
      .SetDeviceMode(event.features().device_mode())
      .SetDeviceType(event.features().device_type())
      .SetEventLogDuration(Bucketize(event.event().log_duration_sec(),
                                     kEventLogDurationBuckets,
                                     arraysize(kEventLogDurationBuckets)))
      .SetEventReason(event.event().reason())
      .SetEventType(event.event().type())
      .SetLastActivityDay(event.features().last_activity_day())
      .SetLastActivityTime(
          std::floor(event.features().last_activity_time_sec() / 3600))
      .SetRecentTimeActive(event.features().recent_time_active_sec())
      .SetRecentVideoPlayingTime(
          Bucketize(event.features().video_playing_time_sec(),
                    kRecentVideoPlayingTimeBuckets,
                    arraysize(kRecentVideoPlayingTimeBuckets)))
      .SetScreenDimmedInitially(event.features().screen_dimmed_initially())
      .SetScreenDimOccurred(event.event().screen_dim_occurred())
      .SetScreenLockedInitially(event.features().screen_locked_initially())
      .SetScreenLockOccurred(event.event().screen_lock_occurred())
      .SetScreenOffInitially(event.features().screen_off_initially())
      .SetScreenOffOccurred(event.event().screen_off_occurred());

  if (event.features().has_on_to_dim_sec()) {
    user_activity.SetScreenDimDelay(event.features().on_to_dim_sec());
  }
  if (event.features().has_dim_to_screen_off_sec()) {
    user_activity.SetScreenDimToOffDelay(
        event.features().dim_to_screen_off_sec());
  }

  if (event.features().has_last_user_activity_time_sec()) {
    user_activity.SetLastUserActivityTime(
        std::floor(event.features().last_user_activity_time_sec() / 3600));
  }
  if (event.features().has_time_since_last_key_sec()) {
    user_activity.SetTimeSinceLastKey(
        event.features().time_since_last_key_sec());
  }
  if (event.features().has_time_since_last_mouse_sec()) {
    user_activity.SetTimeSinceLastMouse(
        event.features().time_since_last_mouse_sec());
  }
  if (event.features().has_time_since_last_touch_sec()) {
    user_activity.SetTimeSinceLastTouch(
        event.features().time_since_last_touch_sec());
  }

  if (event.features().has_on_battery()) {
    user_activity.SetOnBattery(event.features().on_battery());
  }

  if (event.features().has_battery_percent()) {
    user_activity.SetBatteryPercent(
        Bucketize(std::floor(event.features().battery_percent()),
                  kBatteryPercentBuckets, arraysize(kBatteryPercentBuckets)));
  }

  if (event.features().has_device_management()) {
    user_activity.SetDeviceManagement(event.features().device_management());
  }

  if (event.features().has_time_since_video_ended_sec()) {
    user_activity.SetTimeSinceLastVideoEnded(
        Bucketize(event.features().time_since_video_ended_sec(),
                  kTimeSinceLastVideoEndedBuckets,
                  arraysize(kTimeSinceLastVideoEndedBuckets)));
  }

  if (event.features().has_key_events_in_last_hour()) {
    user_activity.SetKeyEventsInLastHour(
        Bucketize(event.features().key_events_in_last_hour(),
                  kUserInputEventBuckets, arraysize(kUserInputEventBuckets)));
  }

  if (event.features().has_mouse_events_in_last_hour()) {
    user_activity.SetMouseEventsInLastHour(
        Bucketize(event.features().mouse_events_in_last_hour(),
                  kUserInputEventBuckets, arraysize(kUserInputEventBuckets)));
  }

  if (event.features().has_touch_events_in_last_hour()) {
    user_activity.SetTouchEventsInLastHour(
        Bucketize(event.features().touch_events_in_last_hour(),
                  kUserInputEventBuckets, arraysize(kUserInputEventBuckets)));
  }

  user_activity.Record(ukm_recorder_);

  for (const std::pair<ukm::SourceId, TabProperty>& kv : open_tabs) {
    const ukm::SourceId& id = kv.first;
    const TabProperty& tab_property = kv.second;
    ukm::builders::UserActivityId user_activity_id(id);
    user_activity_id.SetActivityId(source_id)
        .SetContentType(tab_property.content_type)
        .SetHasFormEntry(tab_property.has_form_entry)
        .SetIsActive(tab_property.is_active)
        .SetIsBrowserFocused(tab_property.is_browser_focused)
        .SetIsBrowserVisible(tab_property.is_browser_visible)
        .SetIsTopmostBrowser(tab_property.is_topmost_browser);
    if (tab_property.engagement_score >= 0) {
      user_activity_id.SetSiteEngagementScore(tab_property.engagement_score);
    }
    user_activity_id.Record(ukm_recorder_);
  }
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos

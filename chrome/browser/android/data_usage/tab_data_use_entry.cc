// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/data_usage/tab_data_use_entry.h"

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/android/data_usage/data_use_tab_model.h"
#include "chrome/browser/android/data_usage/external_data_use_observer.h"
#include "components/variations/variations_associated_data.h"

namespace {

const char kUMATrackingSessionLifetimeHistogram[] =
    "DataUsage.TabModel.TrackingSessionLifetime";
const char kUMAOldInactiveSessionRemovalDurationHistogram[] =
    "DataUsage.TabModel.OldInactiveSessionRemovalDuration";

}  // namespace

namespace android {

TabDataUseEntry::TabDataUseEntry(DataUseTabModel* tab_model)
    : is_custom_tab_package_match_(false), tab_model_(tab_model) {
  DCHECK(tab_model_);
}

TabDataUseEntry::TabDataUseEntry(const TabDataUseEntry& other) = default;

TabDataUseEntry::~TabDataUseEntry() {}

bool TabDataUseEntry::StartTracking(const std::string& label) {
  DCHECK(!label.empty());
  DCHECK(tab_close_time_.is_null());

  if (IsTrackingDataUse())
    return false;  // Duplicate start events.

  // TODO(rajendrant): Explore ability to handle changes in label for current
  // session.

  sessions_.push_back(TabDataUseTrackingSession(label, tab_model_->NowTicks()));

  CompactSessionHistory();
  return true;
}

bool TabDataUseEntry::EndTracking() {
  DCHECK(tab_close_time_.is_null());
  if (!IsTrackingDataUse())
    return false;  // Duplicate end events.

  TabSessions::reverse_iterator back_iterator = sessions_.rbegin();
  if (back_iterator == sessions_.rend())
    return false;

  back_iterator->end_time = tab_model_->NowTicks();

  UMA_HISTOGRAM_CUSTOM_TIMES(
      kUMATrackingSessionLifetimeHistogram,
      back_iterator->end_time - back_iterator->start_time,
      base::TimeDelta::FromSeconds(1), base::TimeDelta::FromHours(1), 50);

  return true;
}

bool TabDataUseEntry::EndTrackingWithLabel(const std::string& label) {
  if (!sessions_.empty() && sessions_.back().label == label)
    return EndTracking();
  return false;
}

void TabDataUseEntry::OnTabCloseEvent() {
  DCHECK(!IsTrackingDataUse());
  tab_close_time_ = tab_model_->NowTicks();
}

bool TabDataUseEntry::IsExpired() const {
  const base::TimeTicks now = tab_model_->NowTicks();

  if (!tab_close_time_.is_null()) {
    // Closed tab entry.
    return ((now - tab_close_time_) >
            tab_model_->closed_tab_expiration_duration());
  }

  const base::TimeTicks latest_session_time = GetLatestStartOrEndTime();
  if (latest_session_time.is_null() ||
      ((now - latest_session_time) >
       tab_model_->open_tab_expiration_duration())) {
    return true;
  }
  return false;
}

bool TabDataUseEntry::GetLabel(const base::TimeTicks& data_use_time,
                               std::string* output_label) const {
  *output_label = "";

  // Find a tracking session in history that was active at |data_use_time|.
  for (TabSessions::const_reverse_iterator session_iterator =
           sessions_.rbegin();
       session_iterator != sessions_.rend(); ++session_iterator) {
    if (session_iterator->start_time <= data_use_time &&
        (session_iterator->end_time.is_null() ||
         session_iterator->end_time >= data_use_time)) {
      *output_label = session_iterator->label;
      return true;
    }
    if (!session_iterator->end_time.is_null() &&
        session_iterator->end_time < data_use_time) {
      // Older sessions in history will end before |data_use_time| and will not
      // match.
      break;
    }
  }
  return false;
}

bool TabDataUseEntry::IsTrackingDataUse() const {
  TabSessions::const_reverse_iterator back_iterator = sessions_.rbegin();
  if (back_iterator == sessions_.rend())
    return false;
  return back_iterator->end_time.is_null();
}

base::TimeTicks TabDataUseEntry::GetLatestStartOrEndTime() const {
  TabSessions::const_reverse_iterator back_iterator = sessions_.rbegin();
  if (back_iterator == sessions_.rend())
    return base::TimeTicks();  // No tab session found.
  if (!back_iterator->end_time.is_null())
    return back_iterator->end_time;

  DCHECK(!back_iterator->start_time.is_null());
  return back_iterator->start_time;
}

std::string TabDataUseEntry::GetActiveTrackingSessionLabel() const {
  TabSessions::const_reverse_iterator back_iterator = sessions_.rbegin();
  if (back_iterator == sessions_.rend() || !IsTrackingDataUse())
    return std::string();
  return back_iterator->label;
}

void TabDataUseEntry::set_custom_tab_package_match(
    bool is_custom_tab_package_match) {
  DCHECK(IsTrackingDataUse());
  DCHECK(!GetActiveTrackingSessionLabel().empty());
  is_custom_tab_package_match_ = is_custom_tab_package_match;
}

void TabDataUseEntry::NotifyPageLoad() {
  DCHECK(IsTrackingDataUse());
  sessions_.back().page_loads++;
  UMA_HISTOGRAM_COUNTS_100("DataUsage.PageLoadSequence",
                           sessions_.back().page_loads);
}

void TabDataUseEntry::CompactSessionHistory() {
  if (sessions_.size() <= tab_model_->max_sessions_per_tab())
    return;

  const auto end_it = sessions_.begin() +
                      (sessions_.size() - tab_model_->max_sessions_per_tab());

  for (auto it = sessions_.begin(); it != end_it; ++it) {
    DCHECK(!it->end_time.is_null());
    // Track how often old sessions are lost.
    UMA_HISTOGRAM_CUSTOM_TIMES(kUMAOldInactiveSessionRemovalDurationHistogram,
                               tab_model_->NowTicks() - it->end_time,
                               base::TimeDelta::FromSeconds(1),
                               base::TimeDelta::FromHours(1), 50);
  }

  sessions_.erase(sessions_.begin(), end_it);
}

}  // namespace android

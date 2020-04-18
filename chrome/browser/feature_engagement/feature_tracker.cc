// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/feature_tracker.h"

#include "base/files/file_util.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/feature_engagement/tracker_factory.h"
#include "chrome/browser/first_run/first_run.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"

namespace {
constexpr double kTwentyFourHoursInSeconds = 86400;
}

namespace feature_engagement {

FeatureTracker::FeatureTracker(
    Profile* profile,
    const base::Feature* feature,
    const char* observed_session_time_dict_key,
    base::TimeDelta default_time_required_to_show_promo)
    : profile_(profile),
      session_duration_updater_(profile->GetPrefs(),
                                observed_session_time_dict_key),
      session_duration_observer_(this),
      feature_(feature),
      field_trial_time_delta_(default_time_required_to_show_promo) {
  if (!HasEnoughSessionTimeElapsed(
          session_duration_updater_.GetCumulativeElapsedSessionTime())) {
    AddSessionDurationObserver();
  }
}

FeatureTracker::~FeatureTracker() = default;

void FeatureTracker::AddSessionDurationObserver() {
  session_duration_observer_.Add(&session_duration_updater_);
}

void FeatureTracker::RemoveSessionDurationObserver() {
  session_duration_observer_.Remove(&session_duration_updater_);
}

bool FeatureTracker::IsObserving() {
  return session_duration_observer_.IsObserving(&session_duration_updater_);
}

bool FeatureTracker::ShouldShowPromo() {
  if (IsObserving()) {
    NotifyAndRemoveSessionDurationObserverIfSessionTimeMet(
        session_duration_updater_.GetCumulativeElapsedSessionTime());
  }

  return IsNewUser() ? GetTracker()->ShouldTriggerHelpUI(*feature_) : false;
}

Tracker* FeatureTracker::GetTracker() const {
  return TrackerFactory::GetForBrowserContext(profile_);
}

void FeatureTracker::OnSessionEnded(base::TimeDelta total_session_time) {
  NotifyAndRemoveSessionDurationObserverIfSessionTimeMet(total_session_time);
}

base::TimeDelta FeatureTracker::GetSessionTimeRequiredToShow() {
  if (!has_retrieved_field_trial_minutes_) {
    has_retrieved_field_trial_minutes_ = true;
    std::string field_trial_string_value =
        base::GetFieldTrialParamValueByFeature(*feature_, "x_minutes");
    int field_trial_int_value;
    if (base::StringToInt(field_trial_string_value, &field_trial_int_value)) {
      field_trial_time_delta_ =
          base::TimeDelta::FromMinutes(field_trial_int_value);
    }
  }
  return field_trial_time_delta_;
}

void FeatureTracker::NotifyAndRemoveSessionDurationObserverIfSessionTimeMet(
    base::TimeDelta total_session_time) {
  if (has_session_time_been_met_ ||
      !HasEnoughSessionTimeElapsed(total_session_time)) {
    return;
  }

  has_session_time_been_met_ = true;
  OnSessionTimeMet();
  RemoveSessionDurationObserver();
}

bool FeatureTracker::HasEnoughSessionTimeElapsed(
    base::TimeDelta total_session_time) {
  return total_session_time.InSeconds() >=
         GetSessionTimeRequiredToShow().InSeconds();
}

bool FeatureTracker::IsNewUser() {
  // Gets the date in seconds since epoch the experiment was released.
  const std::string date_released_string_value =
      base::GetFieldTrialParamValueByFeature(*feature_,
                                             "x_date_released_in_seconds");
  int64_t date_released_int64_value;
  // If the date release string value is incorrect and it's not for testing,
  // directly return false.
  if (!base::StringToInt64(date_released_string_value,
                           &date_released_int64_value)) {
    if (use_default_for_chrome_variation_configuration_release_time_for_testing_) {
      date_released_int64_value = base::Time().ToDoubleT();
    } else {
      return false;
    }
  }

  base::TimeDelta new_user_threshold =
      base::TimeDelta::FromSeconds(kTwentyFourHoursInSeconds);
  // Gets the date in seconds the experiment was released.
  const std::string new_user_threshold_string_value =
      base::GetFieldTrialParamValueByFeature(
          *feature_, "x_new_user_creation_time_threshold_in_seconds");
  int64_t new_user_threshold_int64_value;
  // If the threshold string value is incorrect, return false.
  if (base::StringToInt64(new_user_threshold_string_value,
                          &new_user_threshold_int64_value)) {
    new_user_threshold =
        base::TimeDelta::FromSeconds(new_user_threshold_int64_value);
  } else if (!new_user_threshold_string_value.empty()) {
    return false;
  }

  // We consider a new user only if the first run sentinel has been created no
  // more than 24 hours before the date released.
  return (base::Time::FromDoubleT(date_released_int64_value) -
          first_run::GetFirstRunSentinelCreationTime()) <= new_user_threshold;
}

}  // namespace feature_engagement

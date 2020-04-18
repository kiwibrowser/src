// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_sync/background_sync_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics_action.h"

namespace {

// ResultPattern is used by Histograms, append new entries at the end.
enum ResultPattern {
  RESULT_PATTERN_SUCCESS_FOREGROUND = 0,
  RESULT_PATTERN_SUCCESS_BACKGROUND,
  RESULT_PATTERN_FAILED_FOREGROUND,
  RESULT_PATTERN_FAILED_BACKGROUND,
  RESULT_PATTERN_MAX = RESULT_PATTERN_FAILED_BACKGROUND
};

ResultPattern EventResultToResultPattern(bool success,
                                         bool finished_in_foreground) {
  if (success) {
    return finished_in_foreground ? RESULT_PATTERN_SUCCESS_FOREGROUND
                                  : RESULT_PATTERN_SUCCESS_BACKGROUND;
  }
  return finished_in_foreground ? RESULT_PATTERN_FAILED_FOREGROUND
                                : RESULT_PATTERN_FAILED_BACKGROUND;
}

}  // namespace

namespace content {

// static
void BackgroundSyncMetrics::RecordEventStarted(bool started_in_foreground) {
  UMA_HISTOGRAM_BOOLEAN("BackgroundSync.Event.OneShotStartedInForeground",
                        started_in_foreground);
}

// static
void BackgroundSyncMetrics::RecordEventResult(bool success,
                                              bool finished_in_foreground) {
  UMA_HISTOGRAM_ENUMERATION(
      "BackgroundSync.Event.OneShotResultPattern",
      EventResultToResultPattern(success, finished_in_foreground),
      RESULT_PATTERN_MAX + 1);
}

// static
void BackgroundSyncMetrics::RecordBatchSyncEventComplete(
    const base::TimeDelta& time,
    int number_of_batched_sync_events) {
  // The total batch handling time should be under 5 minutes; we'll record up to
  // 6 minutes, to be safe.
  UMA_HISTOGRAM_CUSTOM_TIMES("BackgroundSync.Event.Time", time,
                             base::TimeDelta::FromMilliseconds(10),
                             base::TimeDelta::FromMinutes(6), 50);
  UMA_HISTOGRAM_COUNTS_100("BackgroundSync.Event.BatchSize",
                           number_of_batched_sync_events);
}

// static
void BackgroundSyncMetrics::CountRegisterSuccess(
    RegistrationCouldFire registration_could_fire,
    RegistrationIsDuplicate registration_is_duplicate) {
  UMA_HISTOGRAM_ENUMERATION("BackgroundSync.Registration.OneShot",
                            BACKGROUND_SYNC_STATUS_OK,
                            BACKGROUND_SYNC_STATUS_MAX + 1);
  UMA_HISTOGRAM_BOOLEAN("BackgroundSync.Registration.OneShot.CouldFire",
                        registration_could_fire == REGISTRATION_COULD_FIRE);
  UMA_HISTOGRAM_BOOLEAN("BackgroundSync.Registration.OneShot.IsDuplicate",
                        registration_is_duplicate == REGISTRATION_IS_DUPLICATE);
  return;
}

// static
void BackgroundSyncMetrics::CountRegisterFailure(BackgroundSyncStatus result) {
  UMA_HISTOGRAM_ENUMERATION("BackgroundSync.Registration.OneShot", result,
                            BACKGROUND_SYNC_STATUS_MAX + 1);
  return;
}

}  // namespace content

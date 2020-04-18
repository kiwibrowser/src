// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/contextual_search/browser/weekly_activity_storage.h"

#include <algorithm>  // std::min

#include "base/logging.h"

namespace {

// Keys for ChromePreferenceManager storage of oldest and newest week written.
const char kOldestWeekWrittenKey[] = "contextual_search_oldest_week";
const char kNewestWeekWrittenKey[] = "contextual_search_newest_week";
// Prefixes for ChromePreferenceManager storage keyed by week.
const char kClicksWeekKeyPrefix[] = "contextual_search_clicks_week_";
const char kImpressionsWeekKeyPrefix[] = "contextual_search_impressions_week_";

// Used for validation in debug build.  Week numbers are > 2300 as of year 2016.
const int kReasonableMinWeek = 2000;

}  // namespace

namespace contextual_search {

WeeklyActivityStorage::WeeklyActivityStorage(int weeks_needed) {
  weeks_needed_ = weeks_needed;
}

WeeklyActivityStorage::~WeeklyActivityStorage() {}

int WeeklyActivityStorage::ReadClicks(int week_number) {
  std::string key = GetWeekClicksKey(week_number);
  return ReadInt(key);
}

void WeeklyActivityStorage::WriteClicks(int week_number, int value) {
  std::string key = GetWeekClicksKey(week_number);
  WriteInt(key, value);
}

int WeeklyActivityStorage::ReadImpressions(int week_number) {
  std::string key = GetWeekImpressionsKey(week_number);
  return ReadInt(key);
}

void WeeklyActivityStorage::WriteImpressions(int week_number, int value) {
  std::string key = GetWeekImpressionsKey(week_number);
  WriteInt(key, value);
}

bool WeeklyActivityStorage::HasData(int week_number) {
  return ReadInt(kOldestWeekWrittenKey) <= week_number &&
         ReadInt(kNewestWeekWrittenKey) >= week_number;
}

void WeeklyActivityStorage::ClearData(int week_number) {
  WriteImpressions(week_number, 0);
  WriteClicks(week_number, 0);
}

void WeeklyActivityStorage::AdvanceToWeek(int week_number) {
  EnsureHasActivity(week_number);
}

// private

std::string WeeklyActivityStorage::GetWeekImpressionsKey(int which_week) {
  return kImpressionsWeekKeyPrefix + GetWeekKey(which_week);
}

std::string WeeklyActivityStorage::GetWeekClicksKey(int which_week) {
  return kClicksWeekKeyPrefix + GetWeekKey(which_week);
}

// Round-robin implementation:
// GetWeekKey and EnsureHasActivity are implemented with a round-robin
// implementation that simply recycles usage of the last N weeks, where N is
// less than weeks_needed_.

std::string WeeklyActivityStorage::GetWeekKey(int which_week) {
  return std::to_string(which_week % (weeks_needed_ + 1));
}

void WeeklyActivityStorage::EnsureHasActivity(int which_week) {
  DCHECK(which_week > kReasonableMinWeek);

  // If still on the newest week we're done!
  int newest_week = ReadInt(kNewestWeekWrittenKey);
  if (newest_week == which_week)
    return;

  // Update the newest and oldest week written.
  if (which_week > newest_week) {
    WriteInt(kNewestWeekWrittenKey, which_week);
  }
  int oldest_week = ReadInt(kOldestWeekWrittenKey);
  if (oldest_week == 0 || oldest_week > which_week)
    WriteInt(kOldestWeekWrittenKey, which_week);

  // Any stale weeks to update?
  if (newest_week == 0)
    return;

  // Moved to some new week beyond the newest previously recorded.
  // Since we recycle storage we must clear the new week and all that we
  // may have skipped since our last access.
  int weeks_to_clear = std::min(which_week - newest_week, weeks_needed_);
  int week = which_week;
  while (weeks_to_clear > 0) {
    WriteInt(GetWeekImpressionsKey(week), 0);
    WriteInt(GetWeekClicksKey(week), 0);
    week--;
    weeks_to_clear--;
  }
}

// Storage access bottlenecks

int WeeklyActivityStorage::ReadInt(std::string storage_bucket) {
  return ReadStorage(storage_bucket);
}

void WeeklyActivityStorage::WriteInt(std::string storage_bucket, int value) {
  WriteStorage(storage_bucket, value);
}

}  // namespace contextual_search

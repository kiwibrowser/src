// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/off_hours/weekly_time.h"
#include "base/time/time.h"

namespace policy {
namespace off_hours {
namespace {

constexpr base::TimeDelta kWeek = base::TimeDelta::FromDays(7);
constexpr base::TimeDelta kDay = base::TimeDelta::FromDays(1);
constexpr base::TimeDelta kHour = base::TimeDelta::FromHours(1);
constexpr base::TimeDelta kMinute = base::TimeDelta::FromMinutes(1);
constexpr base::TimeDelta kSecond = base::TimeDelta::FromSeconds(1);

}  // namespace

WeeklyTime::WeeklyTime(int day_of_week, int milliseconds)
    : day_of_week_(day_of_week), milliseconds_(milliseconds) {
  DCHECK_GT(day_of_week, 0);
  DCHECK_LE(day_of_week, 7);
  DCHECK_GE(milliseconds, 0);
  DCHECK_LT(milliseconds, kDay.InMilliseconds());
}

std::unique_ptr<base::DictionaryValue> WeeklyTime::ToValue() const {
  auto weekly_time = std::make_unique<base::DictionaryValue>();
  weekly_time->SetInteger("day_of_week", day_of_week_);
  weekly_time->SetInteger("time", milliseconds_);
  return weekly_time;
}

base::TimeDelta WeeklyTime::GetDurationTo(const WeeklyTime& other) const {
  int duration = (other.day_of_week() - day_of_week_) * kDay.InMilliseconds() +
                 other.milliseconds() - milliseconds_;
  if (duration < 0)
    duration += kWeek.InMilliseconds();
  return base::TimeDelta::FromMilliseconds(duration);
}

WeeklyTime WeeklyTime::AddMilliseconds(int milliseconds) const {
  milliseconds %= kWeek.InMilliseconds();
  // Make |milliseconds| positive number (add number of milliseconds per week)
  // for easier evaluation.
  milliseconds += kWeek.InMilliseconds();
  int shifted_milliseconds = milliseconds_ + milliseconds;
  // Get milliseconds from the start of the day.
  int result_milliseconds = shifted_milliseconds % kDay.InMilliseconds();
  int day_offset = shifted_milliseconds / kDay.InMilliseconds();
  // Convert day of week considering week is cyclic. +/- 1 is
  // because day of week is from 1 to 7.
  int result_day_of_week = (day_of_week_ + day_offset - 1) % 7 + 1;
  return WeeklyTime(result_day_of_week, result_milliseconds);
}

// static
WeeklyTime WeeklyTime::GetCurrentWeeklyTime(base::Clock* clock) {
  base::Time::Exploded exploded;
  clock->Now().UTCExplode(&exploded);
  int day_of_week = exploded.day_of_week;
  // Exploded contains 0-based day of week (0 = Sunday, etc.)
  if (day_of_week == 0)
    day_of_week = 7;
  return WeeklyTime(day_of_week,
                    exploded.hour * kHour.InMilliseconds() +
                        exploded.minute * kMinute.InMilliseconds() +
                        exploded.second * kSecond.InMilliseconds());
}

}  // namespace off_hours
}  // namespace policy

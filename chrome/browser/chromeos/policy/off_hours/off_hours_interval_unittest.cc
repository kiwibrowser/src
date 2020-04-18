// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/off_hours/off_hours_interval.h"

#include <tuple>
#include <utility>

#include "base/time/time.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {
namespace off_hours {

namespace {

enum {
  kMonday = 1,
  kTuesday = 2,
  kWednesday = 3,
  kThursday = 4,
  kFriday = 5,
  kSaturday = 6,
  kSunday = 7,
};

const int kMinutesInHour = 60;

constexpr base::TimeDelta kMinute = base::TimeDelta::FromMinutes(1);

}  // namespace

class SingleOffHoursIntervalTest
    : public testing::TestWithParam<std::tuple<int, int, int, int>> {
 protected:
  int start_day_of_week() const { return std::get<0>(GetParam()); }
  int start_time() const { return std::get<1>(GetParam()); }
  int end_day_of_week() const { return std::get<2>(GetParam()); }
  int end_time() const { return std::get<3>(GetParam()); }
};

TEST_P(SingleOffHoursIntervalTest, Constructor) {
  WeeklyTime start =
      WeeklyTime(start_day_of_week(), start_time() * kMinute.InMilliseconds());
  WeeklyTime end =
      WeeklyTime(end_day_of_week(), end_time() * kMinute.InMilliseconds());
  OffHoursInterval interval = OffHoursInterval(start, end);
  EXPECT_EQ(interval.start().day_of_week(), start_day_of_week());
  EXPECT_EQ(interval.start().milliseconds(),
            start_time() * kMinute.InMilliseconds());
  EXPECT_EQ(interval.end().day_of_week(), end_day_of_week());
  EXPECT_EQ(interval.end().milliseconds(),
            end_time() * kMinute.InMilliseconds());
}

TEST_P(SingleOffHoursIntervalTest, ToValue) {
  WeeklyTime start = WeeklyTime(start_day_of_week(), start_time());
  WeeklyTime end = WeeklyTime(end_day_of_week(), end_time());
  OffHoursInterval interval = OffHoursInterval(start, end);
  std::unique_ptr<base::DictionaryValue> interval_value = interval.ToValue();
  base::DictionaryValue expected_interval_value;
  expected_interval_value.SetDictionary("start", start.ToValue());
  expected_interval_value.SetDictionary("end", end.ToValue());
  EXPECT_EQ(*interval_value, expected_interval_value);
}

INSTANTIATE_TEST_CASE_P(OneMinuteInterval,
                        SingleOffHoursIntervalTest,
                        testing::Values(std::make_tuple(kWednesday,
                                                        kMinutesInHour,
                                                        kWednesday,
                                                        kMinutesInHour + 1)));
INSTANTIATE_TEST_CASE_P(
    TheLongestInterval,
    SingleOffHoursIntervalTest,
    testing::Values(
        std::make_tuple(kMonday, 0, kSunday, 24 * kMinutesInHour - 1)));

INSTANTIATE_TEST_CASE_P(RandomInterval,
                        SingleOffHoursIntervalTest,
                        testing::Values(std::make_tuple(kTuesday,
                                                        10 * kMinutesInHour,
                                                        kFriday,
                                                        14 * kMinutesInHour +
                                                            15)));

class OffHoursIntervalAndWeeklyTimeTest
    : public testing::TestWithParam<
          std::tuple<int, int, int, int, int, int, bool>> {
 protected:
  int start_day_of_week() const { return std::get<0>(GetParam()); }
  int start_time() const { return std::get<1>(GetParam()); }
  int end_day_of_week() const { return std::get<2>(GetParam()); }
  int end_time() const { return std::get<3>(GetParam()); }
  int current_day_of_week() const { return std::get<4>(GetParam()); }
  int current_time() const { return std::get<5>(GetParam()); }
  bool expected_contains() const { return std::get<6>(GetParam()); }
};

TEST_P(OffHoursIntervalAndWeeklyTimeTest, Contains) {
  WeeklyTime start =
      WeeklyTime(start_day_of_week(), start_time() * kMinute.InMilliseconds());
  WeeklyTime end =
      WeeklyTime(end_day_of_week(), end_time() * kMinute.InMilliseconds());
  OffHoursInterval interval = OffHoursInterval(start, end);
  WeeklyTime weekly_time = WeeklyTime(
      current_day_of_week(), current_time() * kMinute.InMilliseconds());
  EXPECT_EQ(interval.Contains(weekly_time), expected_contains());
}

INSTANTIATE_TEST_CASE_P(TheLongestInterval,
                        OffHoursIntervalAndWeeklyTimeTest,
                        testing::Values(std::make_tuple(kMonday,
                                                        0,
                                                        kSunday,
                                                        24 * kMinutesInHour - 1,
                                                        kWednesday,
                                                        10 * kMinutesInHour,
                                                        true),
                                        std::make_tuple(kSunday,
                                                        24 * kMinutesInHour - 1,
                                                        kMonday,
                                                        0,
                                                        kWednesday,
                                                        10 * kMinutesInHour,
                                                        false)));

INSTANTIATE_TEST_CASE_P(
    TheShortestInterval,
    OffHoursIntervalAndWeeklyTimeTest,
    testing::Values(std::make_tuple(kMonday,
                                    0,
                                    kMonday,
                                    1,
                                    kTuesday,
                                    9 * kMinutesInHour,
                                    false),
                    std::make_tuple(kMonday, 0, kMonday, 1, kMonday, 1, false),
                    std::make_tuple(kMonday, 0, kMonday, 1, kMonday, 0, true)));

INSTANTIATE_TEST_CASE_P(
    CheckStartInterval,
    OffHoursIntervalAndWeeklyTimeTest,
    testing::Values(std::make_tuple(kTuesday,
                                    10 * kMinutesInHour + 30,
                                    kFriday,
                                    14 * kMinutesInHour + 45,
                                    kTuesday,
                                    10 * kMinutesInHour + 30,
                                    true)));

INSTANTIATE_TEST_CASE_P(
    CheckEndInterval,
    OffHoursIntervalAndWeeklyTimeTest,
    testing::Values(std::make_tuple(kTuesday,
                                    10 * kMinutesInHour + 30,
                                    kFriday,
                                    14 * kMinutesInHour + 45,
                                    kFriday,
                                    14 * kMinutesInHour + 45,
                                    false)));

INSTANTIATE_TEST_CASE_P(RandomInterval,
                        OffHoursIntervalAndWeeklyTimeTest,
                        testing::Values(std::make_tuple(kFriday,
                                                        17 * kMinutesInHour +
                                                            60,
                                                        kMonday,
                                                        9 * kMinutesInHour,
                                                        kSunday,
                                                        14 * kMinutesInHour,
                                                        true),
                                        std::make_tuple(kMonday,
                                                        9 * kMinutesInHour,
                                                        kFriday,
                                                        17 * kMinutesInHour,
                                                        kSunday,
                                                        14 * kMinutesInHour,
                                                        false)));

}  // namespace off_hours
}  // namespace policy

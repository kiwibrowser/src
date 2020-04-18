// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/off_hours/off_hours_proto_parser.h"

#include "base/logging.h"
#include "base/time/time.h"

namespace em = enterprise_management;

namespace policy {
namespace off_hours {
namespace {
constexpr base::TimeDelta kDay = base::TimeDelta::FromDays(1);
}  // namespace

std::unique_ptr<WeeklyTime> ExtractWeeklyTimeFromProto(
    const em::WeeklyTimeProto& container) {
  if (!container.has_day_of_week() ||
      container.day_of_week() == em::WeeklyTimeProto::DAY_OF_WEEK_UNSPECIFIED) {
    LOG(ERROR) << "Day of week in interval is absent or unspecified.";
    return nullptr;
  }
  if (!container.has_time()) {
    LOG(ERROR) << "Time in interval is absent.";
    return nullptr;
  }
  int time_of_day = container.time();
  if (!(time_of_day >= 0 && time_of_day < kDay.InMilliseconds())) {
    LOG(ERROR) << "Invalid time value: " << time_of_day
               << ", the value should be in [0; " << kDay.InMilliseconds()
               << ").";
    return nullptr;
  }
  return std::make_unique<WeeklyTime>(container.day_of_week(), time_of_day);
}

std::vector<OffHoursInterval> ExtractOffHoursIntervalsFromProto(
    const em::DeviceOffHoursProto& container) {
  std::vector<OffHoursInterval> intervals;
  for (const auto& entry : container.intervals()) {
    if (!entry.has_start() || !entry.has_end()) {
      LOG(WARNING) << "Skipping interval without start or/and end.";
      continue;
    }
    auto start = ExtractWeeklyTimeFromProto(entry.start());
    auto end = ExtractWeeklyTimeFromProto(entry.end());
    if (start && end)
      intervals.push_back(OffHoursInterval(*start, *end));
  }
  return intervals;
}

std::vector<int> ExtractIgnoredPolicyProtoTagsFromProto(
    const em::DeviceOffHoursProto& container) {
  return std::vector<int>(container.ignored_policy_proto_tags().begin(),
                          container.ignored_policy_proto_tags().end());
}

base::Optional<std::string> ExtractTimezoneFromProto(
    const em::DeviceOffHoursProto& container) {
  if (!container.has_timezone()) {
    return base::nullopt;
  }
  return base::make_optional(container.timezone());
}

std::unique_ptr<base::DictionaryValue> ConvertOffHoursProtoToValue(
    const em::DeviceOffHoursProto& container) {
  base::Optional<std::string> timezone = ExtractTimezoneFromProto(container);
  if (!timezone)
    return nullptr;
  auto off_hours = std::make_unique<base::DictionaryValue>();
  off_hours->SetString("timezone", *timezone);
  std::vector<OffHoursInterval> intervals =
      ExtractOffHoursIntervalsFromProto(container);
  auto intervals_value = std::make_unique<base::ListValue>();
  for (const auto& interval : intervals)
    intervals_value->Append(interval.ToValue());
  off_hours->SetList("intervals", std::move(intervals_value));
  std::vector<int> ignored_policy_proto_tags =
      ExtractIgnoredPolicyProtoTagsFromProto(container);
  auto ignored_policies_value = std::make_unique<base::ListValue>();
  for (const auto& policy : ignored_policy_proto_tags)
    ignored_policies_value->GetList().emplace_back(policy);
  off_hours->SetList("ignored_policy_proto_tags",
                     std::move(ignored_policies_value));
  return off_hours;
}

}  // namespace off_hours
}  // namespace policy

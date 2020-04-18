// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_OFF_HOURS_PROTO_PARSER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_OFF_HOURS_PROTO_PARSER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/optional.h"
#include "base/values.h"
#include "chrome/browser/chromeos/policy/off_hours/off_hours_interval.h"
#include "chrome/browser/chromeos/policy/off_hours/weekly_time.h"
#include "components/policy/proto/chrome_device_policy.pb.h"

namespace policy {
namespace off_hours {

// Return WeeklyTime structure from WeeklyTimeProto. Return nullptr if
// WeeklyTime structure isn't correct.
std::unique_ptr<WeeklyTime> ExtractWeeklyTimeFromProto(
    const enterprise_management::WeeklyTimeProto& container);

// Return list of time intervals from DeviceOffHoursProto structure.
std::vector<OffHoursInterval> ExtractOffHoursIntervalsFromProto(
    const enterprise_management::DeviceOffHoursProto& container);

// Return list of proto tags of ignored policies from DeviceOffHoursProto
// structure.
std::vector<int> ExtractIgnoredPolicyProtoTagsFromProto(
    const enterprise_management::DeviceOffHoursProto& container);

// Return timezone from DeviceOffHoursProto if exists otherwise return nullptr.
base::Optional<std::string> ExtractTimezoneFromProto(
    const enterprise_management::DeviceOffHoursProto& container);

// Return DictionaryValue in format:
// { "timezone" : string,
//   "intervals" : list of "OffHours" Intervals,
//   "ignored_policy_proto_tags" : integer list }
// "OffHours" Interval dictionary format:
// { "start" : WeeklyTime,
//   "end" : WeeklyTime }
// WeeklyTime dictionary format:
// { "day_of_week" : int # value is from 1 to 7 (1 = Monday, 2 = Tuesday, etc.)
//   "time" : int # in milliseconds from the beginning of the day.
// }
// This function is used by device_policy_decoder_chromeos to save "OffHours"
// policy in PolicyMap.
std::unique_ptr<base::DictionaryValue> ConvertOffHoursProtoToValue(
    const enterprise_management::DeviceOffHoursProto& container);

}  // namespace off_hours
}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_OFF_HOURS_PROTO_PARSER_H_

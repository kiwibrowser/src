// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_TIME_UTILS_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_TIME_UTILS_H_

#include <string>
#include <vector>

#include "chrome/browser/chromeos/policy/off_hours/off_hours_interval.h"
#include "chrome/browser/chromeos/policy/off_hours/weekly_time.h"

namespace policy {
namespace off_hours {

// Put time in milliseconds which is added to local time to get GMT time to
// |offset| considering daylight from |clock|. Return true if there was no
// error.
bool GetOffsetFromTimezoneToGmt(const std::string& timezone,
                                base::Clock* clock,
                                int* offset);

// Convert time intervals from |timezone| to GMT timezone.
std::vector<OffHoursInterval> ConvertIntervalsToGmt(
    const std::vector<OffHoursInterval>& intervals,
    base::Clock* clock,
    const std::string& timezone);

// Return duration till next "OffHours" time interval.
base::TimeDelta GetDeltaTillNextOffHours(
    const WeeklyTime& current_time,
    const std::vector<OffHoursInterval>& off_hours_intervals);

}  // namespace off_hours
}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_TIME_UTILS_H_

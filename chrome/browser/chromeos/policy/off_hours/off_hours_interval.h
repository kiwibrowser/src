// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_OFF_HOURS_INTERVAL_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_OFF_HOURS_INTERVAL_H_

#include <memory>

#include "base/values.h"
#include "chrome/browser/chromeos/policy/off_hours/weekly_time.h"

namespace policy {
namespace off_hours {

// Represents non-emply time interval [start, end) between two weekly times.
// Interval can be wrapped across the end of the week.
// Interval is empty if start = end. Empty intervals isn't allowed.
class OffHoursInterval {
 public:
  OffHoursInterval(const WeeklyTime& start, const WeeklyTime& end);

  // Return DictionaryValue in format:
  // { "start" : WeeklyTime,
  //   "end" : WeeklyTime }
  // WeeklyTime dictionary format:
  // { "day_of_week" : int # value is from 1 to 7 (1 = Monday, 2 = Tuesday,
  // etc.)
  //   "time" : int # in milliseconds from the beginning of the day.
  // }
  std::unique_ptr<base::DictionaryValue> ToValue() const;

  // Check if |w| is in [OffHoursInterval.start, OffHoursInterval.end). |end|
  // time is always after |start| time. It's possible because week time is
  // cyclic. (i.e. [Friday 17:00, Monday 9:00) )
  bool Contains(const WeeklyTime& w) const;

  WeeklyTime start() const { return start_; }

  WeeklyTime end() const { return end_; }

 private:
  WeeklyTime start_;
  WeeklyTime end_;
};

}  // namespace off_hours
}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_OFF_HOURS_OFF_HOURS_INTERVAL_H_

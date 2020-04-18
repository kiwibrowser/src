/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_DATE_COMPONENTS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_DATE_COMPONENTS_H_

#include <limits>
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/unicode.h"

namespace blink {

// A DateComponents instance represents one of the following date and time
// combinations:
// * Month type: year-month
// * Date type: year-month-day
// * Week type: year-week
// * Time type: hour-minute-second-millisecond
// * DateTime or DateTimeLocal type:
//   year-month-day hour-minute-second-millisecond
class PLATFORM_EXPORT DateComponents {
  DISALLOW_NEW();

 public:
  DateComponents()
      : millisecond_(0),
        second_(0),
        minute_(0),
        hour_(0),
        month_day_(0),
        month_(0),
        year_(0),
        week_(0),
        type_(kInvalid) {}

  enum Type {
    kInvalid,
    kDate,
    kDateTime,
    kDateTimeLocal,
    kMonth,
    kTime,
    kWeek,
  };

  int Millisecond() const { return millisecond_; }
  int Second() const { return second_; }
  int Minute() const { return minute_; }
  int Hour() const { return hour_; }
  int MonthDay() const { return month_day_; }
  int WeekDay() const;
  int Month() const { return month_; }
  int FullYear() const { return year_; }
  int Week() const { return week_; }
  Type GetType() const { return type_; }

  enum SecondFormat {
    kNone,  // Suppress the second part and the millisecond part if they are 0.
    kSecond,  // Always show the second part, and suppress the millisecond part
              // if it is 0.
    kMillisecond  // Always show the second part and the millisecond part.
  };

  // Returns an ISO 8601 representation for this instance.
  // The format argument is valid for DateTime, DateTimeLocal, and Time types.
  String ToString(SecondFormat format = kNone) const;

  // parse*() and setMillisecondsSince*() functions are initializers for an
  // DateComponents instance. If these functions return false, the instance
  // might be invalid.

  // The following six functions parse the input 'src' whose length is
  // 'length', and updates some fields of this instance. The parsing starts at
  // src[start] and examines characters before src[length].
  // 'src' must be non-null. The 'src' string doesn't need to be
  // null-terminated.
  // The functions return true if the parsing succeeds, and set 'end' to the
  // next index after the last consumed. Extra leading characters cause parse
  // failures, and the trailing extra characters don't cause parse failures.

  // Sets year and month.
  bool ParseMonth(const String&, unsigned start, unsigned& end);
  // Sets year, month and monthDay.
  bool ParseDate(const String&, unsigned start, unsigned& end);
  // Sets year and week.
  bool ParseWeek(const String&, unsigned start, unsigned& end);
  // Sets hour, minute, second and millisecond.
  bool ParseTime(const String&, unsigned start, unsigned& end);
  // Sets year, month, monthDay, hour, minute, second and millisecond.
  bool ParseDateTimeLocal(const String&, unsigned start, unsigned& end);

  // The following setMillisecondsSinceEpochFor*() functions take
  // the number of milliseconds since 1970-01-01 00:00:00.000 UTC as
  // the argument, and update all fields for the corresponding
  // DateComponents type. The functions return true if it succeeds, and
  // false if they fail.

  // For Date type. Updates m_year, m_month and m_monthDay.
  bool SetMillisecondsSinceEpochForDate(double ms);
  // For DateTime type. Updates m_year, m_month, m_monthDay, m_hour, m_minute,
  // m_second and m_millisecond.
  bool SetMillisecondsSinceEpochForDateTime(double ms);
  // For DateTimeLocal type. Updates m_year, m_month, m_monthDay, m_hour,
  // m_minute, m_second and m_millisecond.
  bool SetMillisecondsSinceEpochForDateTimeLocal(double ms);
  // For Month type. Updates m_year and m_month.
  bool SetMillisecondsSinceEpochForMonth(double ms);
  // For Week type. Updates m_year and m_week.
  bool SetMillisecondsSinceEpochForWeek(double ms);

  // For Time type. Updates m_hour, m_minute, m_second and m_millisecond.
  bool SetMillisecondsSinceMidnight(double ms);

  // Another initializer for Month type. Updates m_year and m_month.
  bool SetMonthsSinceEpoch(double months);
  // Another initializer for Week type. Updates m_year and m_week.
  bool SetWeek(int year, int week_number);

  // Returns the number of milliseconds from 1970-01-01 00:00:00 UTC.
  // For a DateComponents initialized with parseDateTimeLocal(),
  // millisecondsSinceEpoch() returns a value for UTC timezone.
  double MillisecondsSinceEpoch() const;
  // Returns the number of months from 1970-01.
  // Do not call this for types other than Month.
  double MonthsSinceEpoch() const;
  static inline double InvalidMilliseconds() {
    return std::numeric_limits<double>::quiet_NaN();
  }

  // Minimum and maxmimum limits for setMillisecondsSince*(),
  // setMonthsSinceEpoch(), millisecondsSinceEpoch(), and monthsSinceEpoch().
  static inline double MinimumDate() {
    return -62135596800000.0;
  }  // 0001-01-01T00:00Z
  static inline double MinimumDateTime() {
    return -62135596800000.0;
  }  // ditto.
  static inline double MinimumMonth() {
    return (1 - 1970) * 12.0 + 1 - 1;
  }                                                 // 0001-01
  static inline double MinimumTime() { return 0; }  // 00:00:00.000
  static inline double MinimumWeek() {
    return -62135596800000.0;
  }  // 0001-01-01, the first Monday of 0001.
  static inline double MaximumDate() {
    return 8640000000000000.0;
  }  // 275760-09-13T00:00Z
  static inline double MaximumDateTime() {
    return 8640000000000000.0;
  }  // ditto.
  static inline double MaximumMonth() {
    return (275760 - 1970) * 12.0 + 9 - 1;
  }                                                        // 275760-09
  static inline double MaximumTime() { return 86399999; }  // 23:59:59.999
  static inline double MaximumWeek() {
    return 8639999568000000.0;
  }  // 275760-09-08, the Monday of the week including 275760-09-13.

  // HTML5 uses ISO-8601 format with year >= 1. Gregorian calendar started in
  // 1582. However, we need to support 0001-01-01 in Gregorian calendar rule.
  static inline int MinimumYear() { return 1; }
  // Date in ECMAScript can't represent dates later than 275760-09-13T00:00Z.
  // So, we have the same upper limit in HTML5 date/time types.
  static inline int MaximumYear() { return 275760; }
  static const int kMinimumWeekNumber;
  static const int kMaximumWeekNumber;

 private:
  // Returns the maximum week number in this DateComponents's year.
  // The result is either of 52 and 53.
  int MaxWeekNumberInYear() const;
  bool ParseYear(const String&, unsigned start, unsigned& end);
  bool AddDay(int);
  bool AddMinute(int);
  bool ParseTimeZone(const String&, unsigned start, unsigned& end);
  // Helper for millisecondsSinceEpoch().
  double MillisecondsSinceEpochForTime() const;
  // Helpers for setMillisecondsSinceEpochFor*().
  bool SetMillisecondsSinceEpochForDateInternal(double ms);
  void SetMillisecondsSinceMidnightInternal(double ms);
  // Helper for toString().
  String ToStringForTime(SecondFormat) const;

  // m_weekDay values
  enum {
    kSunday = 0,
    kMonday,
    kTuesday,
    kWednesday,
    kThursday,
    kFriday,
    kSaturday,
  };

  int millisecond_;  // 0 - 999
  int second_;
  int minute_;
  int hour_;
  int month_day_;  // 1 - 31
  int month_;      // 0:January - 11:December
  int year_;       //  1582 -
  int week_;       // 1 - 53

  Type type_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_DATE_COMPONENTS_H_

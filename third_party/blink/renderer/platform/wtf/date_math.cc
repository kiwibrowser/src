/*
 * Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2010 &yet, LLC. (nate@andyet.net)
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.

 * Copyright 2006-2008 the V8 project authors. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Google Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#include "third_party/blink/renderer/platform/wtf/date_math.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <limits>
#include "build/build_config.h"
#include "third_party/blink/renderer/platform/wtf/ascii_ctype.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/string_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

#if defined(OS_WIN)
#include <windows.h>
#else
#include <sys/time.h>
#endif

namespace WTF {

/* Constants */

static const double kHoursPerDay = 24.0;
static const double kSecondsPerDay = 24.0 * 60.0 * 60.0;

static const double kMaxUnixTime = 2145859200.0;  // 12/31/2037
static const double kMinimumECMADateInMs = -8640000000000000.0;
static const double kMaximumECMADateInMs = 8640000000000000.0;

// Day of year for the first day of each month, where index 0 is January, and
// day 0 is January 1.  First for non-leap years, then for leap years.
static const int kFirstDayOfMonth[2][12] = {
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}};

static inline void GetLocalTime(const time_t* local_time, struct tm* local_tm) {
#if defined(COMPILER_MSVC)
  localtime_s(local_tm, local_time);
#else
  localtime_r(local_time, local_tm);
#endif
}

bool IsLeapYear(int year) {
  if (year % 4 != 0)
    return false;
  if (year % 400 == 0)
    return true;
  if (year % 100 == 0)
    return false;
  return true;
}

static inline int DaysInYear(int year) {
  return 365 + IsLeapYear(year);
}

static inline double DaysFrom1970ToYear(int year) {
  // The Gregorian Calendar rules for leap years:
  // Every fourth year is a leap year.  2004, 2008, and 2012 are leap years.
  // However, every hundredth year is not a leap year.  1900 and 2100 are not
  // leap years.
  // Every four hundred years, there's a leap year after all.  2000 and 2400 are
  // leap years.

  static const int kLeapDaysBefore1971By4Rule = 1970 / 4;
  static const int kExcludedLeapDaysBefore1971By100Rule = 1970 / 100;
  static const int kLeapDaysBefore1971By400Rule = 1970 / 400;

  const double year_minus_one = year - 1;
  const double years_to_add_by4_rule =
      floor(year_minus_one / 4.0) - kLeapDaysBefore1971By4Rule;
  const double years_to_exclude_by100_rule =
      floor(year_minus_one / 100.0) - kExcludedLeapDaysBefore1971By100Rule;
  const double years_to_add_by400_rule =
      floor(year_minus_one / 400.0) - kLeapDaysBefore1971By400Rule;

  return 365.0 * (year - 1970) + years_to_add_by4_rule -
         years_to_exclude_by100_rule + years_to_add_by400_rule;
}

static double MsToDays(double ms) {
  return floor(ms / kMsPerDay);
}

static void AppendTwoDigitNumber(StringBuilder& builder, int number) {
  DCHECK_GE(number, 0);
  DCHECK_LT(number, 100);
  if (number <= 9)
    builder.Append('0');
  builder.AppendNumber(number);
}

int MsToYear(double ms) {
  DCHECK(std::isfinite(ms));
  DCHECK_GE(ms, kMinimumECMADateInMs);
  DCHECK_LE(ms, kMaximumECMADateInMs);
  int approx_year = static_cast<int>(floor(ms / (kMsPerDay * 365.2425)) + 1970);
  double ms_from_approx_year_to1970 =
      kMsPerDay * DaysFrom1970ToYear(approx_year);
  if (ms_from_approx_year_to1970 > ms)
    return approx_year - 1;
  if (ms_from_approx_year_to1970 + kMsPerDay * DaysInYear(approx_year) <= ms)
    return approx_year + 1;
  return approx_year;
}

int DayInYear(double ms, int year) {
  return static_cast<int>(MsToDays(ms) - DaysFrom1970ToYear(year));
}

static inline double MsToMilliseconds(double ms) {
  double result = fmod(ms, kMsPerDay);
  if (result < 0)
    result += kMsPerDay;
  return result;
}

int MonthFromDayInYear(int day_in_year, bool leap_year) {
  const int d = day_in_year;
  int step;

  if (d < (step = 31))
    return 0;
  step += (leap_year ? 29 : 28);
  if (d < step)
    return 1;
  if (d < (step += 31))
    return 2;
  if (d < (step += 30))
    return 3;
  if (d < (step += 31))
    return 4;
  if (d < (step += 30))
    return 5;
  if (d < (step += 31))
    return 6;
  if (d < (step += 31))
    return 7;
  if (d < (step += 30))
    return 8;
  if (d < (step += 31))
    return 9;
  if (d < (step += 30))
    return 10;
  return 11;
}

static inline bool CheckMonth(int day_in_year,
                              int& start_day_of_this_month,
                              int& start_day_of_next_month,
                              int days_in_this_month) {
  start_day_of_this_month = start_day_of_next_month;
  start_day_of_next_month += days_in_this_month;
  return (day_in_year <= start_day_of_next_month);
}

int DayInMonthFromDayInYear(int day_in_year, bool leap_year) {
  const int d = day_in_year;
  int step;
  int next = 30;

  if (d <= next)
    return d + 1;
  const int days_in_feb = (leap_year ? 29 : 28);
  if (CheckMonth(d, step, next, days_in_feb))
    return d - step;
  if (CheckMonth(d, step, next, 31))
    return d - step;
  if (CheckMonth(d, step, next, 30))
    return d - step;
  if (CheckMonth(d, step, next, 31))
    return d - step;
  if (CheckMonth(d, step, next, 30))
    return d - step;
  if (CheckMonth(d, step, next, 31))
    return d - step;
  if (CheckMonth(d, step, next, 31))
    return d - step;
  if (CheckMonth(d, step, next, 30))
    return d - step;
  if (CheckMonth(d, step, next, 31))
    return d - step;
  if (CheckMonth(d, step, next, 30))
    return d - step;
  step = next;
  return d - step;
}

int DayInYear(int year, int month, int day) {
  return kFirstDayOfMonth[IsLeapYear(year)][month] + day - 1;
}

double DateToDaysFrom1970(int year, int month, int day) {
  year += month / 12;

  month %= 12;
  if (month < 0) {
    month += 12;
    --year;
  }

  double yearday = floor(DaysFrom1970ToYear(year));
  DCHECK((year >= 1970 && yearday >= 0) || (year < 1970 && yearday < 0));
  return yearday + DayInYear(year, month, day);
}

// There is a hard limit at 2038 that we currently do not have a workaround
// for (rdar://problem/5052975).
static inline int MaximumYearForDST() {
  return 2037;
}

static inline double JsCurrentTime() {
  // JavaScript doesn't recognize fractions of a millisecond.
  return floor(WTF::CurrentTimeMS());
}

static inline int MinimumYearForDST() {
  // Because of the 2038 issue (see maximumYearForDST) if the current year is
  // greater than the max year minus 27 (2010), we want to use the max year
  // minus 27 instead, to ensure there is a range of 28 years that all years
  // can map to.
  return std::min(MsToYear(JsCurrentTime()), MaximumYearForDST() - 27);
}

// Find an equivalent year for the one given, where equivalence is deterined by
// the two years having the same leapness and the first day of the year, falling
// on the same day of the week.
//
// This function returns a year between this current year and 2037, however this
// function will potentially return incorrect results if the current year is
// after 2010, (rdar://problem/5052975), if the year passed in is before 1900
// or after 2100, (rdar://problem/5055038).
static int EquivalentYearForDST(int year) {
  // It is ok if the cached year is not the current year as long as the rules
  // for DST did not change between the two years; if they did the app would
  // need to be restarted.
  static int min_year = MinimumYearForDST();
  int max_year = MaximumYearForDST();

  int difference;
  if (year > max_year)
    difference = min_year - year;
  else if (year < min_year)
    difference = max_year - year;
  else
    return year;

  int quotient = difference / 28;
  int product = (quotient)*28;

  year += product;
  DCHECK((year >= min_year && year <= max_year) ||
         (product - year ==
          static_cast<int>(std::numeric_limits<double>::quiet_NaN())));
  return year;
}

static double CalculateUTCOffset() {
#if defined(OS_WIN)
  TIME_ZONE_INFORMATION time_zone_information;
  GetTimeZoneInformation(&time_zone_information);
  int32_t bias =
      time_zone_information.Bias + time_zone_information.StandardBias;
  return -bias * 60 * 1000;
#else
  time_t local_time = time(nullptr);
  tm localt;
  GetLocalTime(&local_time, &localt);

  // tm_gmtoff includes any daylight savings offset, so subtract it.
  return static_cast<double>(localt.tm_gmtoff * kMsPerSecond -
                             (localt.tm_isdst > 0 ? kMsPerHour : 0));
#endif
}

/*
 * Get the DST offset for the time passed in.
 */
static double CalculateDSTOffsetSimple(double local_time_seconds,
                                       double utc_offset) {
  if (local_time_seconds > kMaxUnixTime)
    local_time_seconds = kMaxUnixTime;
  else if (local_time_seconds <
           0)  // Go ahead a day to make localtime work (does not work with 0)
    local_time_seconds += kSecondsPerDay;

  // FIXME: time_t has a potential problem in 2038
  time_t local_time = static_cast<time_t>(local_time_seconds);

  tm local_tm;
  GetLocalTime(&local_time, &local_tm);

  return local_tm.tm_isdst > 0 ? kMsPerHour : 0;
}

// Get the DST offset, given a time in UTC
static double CalculateDSTOffset(double ms, double utc_offset) {
  // On macOS, the call to localtime (see calculateDSTOffsetSimple) will return
  // historically accurate DST information (e.g. New Zealand did not have DST
  // from 1946 to 1974) however the JavaScript standard explicitly dictates
  // that historical information should not be considered when determining DST.
  // For this reason we shift away from years that localtime can handle but
  // would return historically accurate information.
  int year = MsToYear(ms);
  int equivalent_year = EquivalentYearForDST(year);
  if (year != equivalent_year) {
    bool leap_year = IsLeapYear(year);
    int day_in_year_local = DayInYear(ms, year);
    int day_in_month = DayInMonthFromDayInYear(day_in_year_local, leap_year);
    int month = MonthFromDayInYear(day_in_year_local, leap_year);
    double day = DateToDaysFrom1970(equivalent_year, month, day_in_month);
    ms = (day * kMsPerDay) + MsToMilliseconds(ms);
  }

  return CalculateDSTOffsetSimple(ms / kMsPerSecond, utc_offset);
}

void InitializeDates() {
#if DCHECK_IS_ON()
  static bool already_initialized;
  DCHECK(!already_initialized);
  already_initialized = true;
#endif

  EquivalentYearForDST(
      2000);  // Need to call once to initialize a static used in this function.
}

static inline double YmdhmsToSeconds(int year,
                                     long mon,
                                     long day,
                                     long hour,
                                     long minute,
                                     double second) {
  double days =
      (day - 32075) + floor(1461 * (year + 4800.0 + (mon - 14) / 12) / 4) +
      367 * (mon - 2 - (mon - 14) / 12 * 12) / 12 -
      floor(3 * ((year + 4900.0 + (mon - 14) / 12) / 100) / 4) - 2440588;
  return ((days * kHoursPerDay + hour) * kMinutesPerHour + minute) *
             kSecondsPerMinute +
         second;
}

// We follow the recommendation of RFC 2822 to consider all
// obsolete time zones not listed here equivalent to "-0000".
static const struct KnownZone {
#if !defined(OS_WIN)
  const
#endif
      char tz_name[4];
  int tz_offset;
} known_zones[] = {{"UT", 0},     {"GMT", 0},    {"EST", -300}, {"EDT", -240},
                   {"CST", -360}, {"CDT", -300}, {"MST", -420}, {"MDT", -360},
                   {"PST", -480}, {"PDT", -420}};

inline static void SkipSpacesAndComments(const char*& s) {
  int nesting = 0;
  char ch;
  while ((ch = *s)) {
    if (!IsASCIISpace(ch)) {
      if (ch == '(')
        nesting++;
      else if (ch == ')' && nesting > 0)
        nesting--;
      else if (nesting == 0)
        break;
    }
    s++;
  }
}

// returns 0-11 (Jan-Dec); -1 on failure
static int FindMonth(const char* month_str) {
  DCHECK(month_str);
  char needle[4];
  for (int i = 0; i < 3; ++i) {
    if (!*month_str)
      return -1;
    needle[i] = static_cast<char>(ToASCIILower(*month_str++));
  }
  needle[3] = '\0';
  const char* haystack = "janfebmaraprmayjunjulaugsepoctnovdec";
  const char* str = strstr(haystack, needle);
  if (str) {
    int position = static_cast<int>(str - haystack);
    if (position % 3 == 0)
      return position / 3;
  }
  return -1;
}

static bool ParseInt(const char* string,
                     char** stop_position,
                     int base,
                     int* result) {
  long long_result = strtol(string, stop_position, base);
  // Avoid the use of errno as it is not available on Windows CE
  if (string == *stop_position ||
      long_result <= std::numeric_limits<int>::min() ||
      long_result >= std::numeric_limits<int>::max())
    return false;
  *result = static_cast<int>(long_result);
  return true;
}

static bool ParseLong(const char* string,
                      char** stop_position,
                      int base,
                      long* result) {
  *result = strtol(string, stop_position, base);
  // Avoid the use of errno as it is not available on Windows CE
  if (string == *stop_position || *result == std::numeric_limits<long>::min() ||
      *result == std::numeric_limits<long>::max())
    return false;
  return true;
}

// Odd case where 'exec' is allowed to be 0, to accomodate a caller in WebCore.
static double ParseDateFromNullTerminatedCharacters(const char* date_string,
                                                    bool& have_tz,
                                                    int& offset) {
  have_tz = false;
  offset = 0;

  // This parses a date in the form:
  //     Tuesday, 09-Nov-99 23:12:40 GMT
  // or
  //     Sat, 01-Jan-2000 08:00:00 GMT
  // or
  //     Sat, 01 Jan 2000 08:00:00 GMT
  // or
  //     01 Jan 99 22:00 +0100    (exceptions in rfc822/rfc2822)
  // ### non RFC formats, added for Javascript:
  //     [Wednesday] January 09 1999 23:12:40 GMT
  //     [Wednesday] January 09 23:12:40 GMT 1999
  //
  // We ignore the weekday.

  // Skip leading space
  SkipSpacesAndComments(date_string);

  long month = -1;
  const char* word_start = date_string;
  // Check contents of first words if not number
  while (*date_string && !IsASCIIDigit(*date_string)) {
    if (IsASCIISpace(*date_string) || *date_string == '(') {
      if (date_string - word_start >= 3)
        month = FindMonth(word_start);
      SkipSpacesAndComments(date_string);
      word_start = date_string;
    } else {
      date_string++;
    }
  }

  // Missing delimiter between month and day (like "January29")?
  if (month == -1 && word_start != date_string)
    month = FindMonth(word_start);

  SkipSpacesAndComments(date_string);

  if (!*date_string)
    return std::numeric_limits<double>::quiet_NaN();

  // ' 09-Nov-99 23:12:40 GMT'
  char* new_pos_str;
  long day;
  if (!ParseLong(date_string, &new_pos_str, 10, &day))
    return std::numeric_limits<double>::quiet_NaN();
  date_string = new_pos_str;

  if (!*date_string)
    return std::numeric_limits<double>::quiet_NaN();

  if (day < 0)
    return std::numeric_limits<double>::quiet_NaN();

  int year = 0;
  if (day > 31) {
    // ### where is the boundary and what happens below?
    if (*date_string != '/')
      return std::numeric_limits<double>::quiet_NaN();
    // looks like a YYYY/MM/DD date
    if (!*++date_string)
      return std::numeric_limits<double>::quiet_NaN();
    if (day <= std::numeric_limits<int>::min() ||
        day >= std::numeric_limits<int>::max())
      return std::numeric_limits<double>::quiet_NaN();
    year = static_cast<int>(day);
    if (!ParseLong(date_string, &new_pos_str, 10, &month))
      return std::numeric_limits<double>::quiet_NaN();
    month -= 1;
    date_string = new_pos_str;
    if (*date_string++ != '/' || !*date_string)
      return std::numeric_limits<double>::quiet_NaN();
    if (!ParseLong(date_string, &new_pos_str, 10, &day))
      return std::numeric_limits<double>::quiet_NaN();
    date_string = new_pos_str;
  } else if (*date_string == '/' && month == -1) {
    date_string++;
    // This looks like a MM/DD/YYYY date, not an RFC date.
    month = day - 1;  // 0-based
    if (!ParseLong(date_string, &new_pos_str, 10, &day))
      return std::numeric_limits<double>::quiet_NaN();
    if (day < 1 || day > 31)
      return std::numeric_limits<double>::quiet_NaN();
    date_string = new_pos_str;
    if (*date_string == '/')
      date_string++;
    if (!*date_string)
      return std::numeric_limits<double>::quiet_NaN();
  } else {
    if (*date_string == '-')
      date_string++;

    SkipSpacesAndComments(date_string);

    if (*date_string == ',')
      date_string++;

    if (month == -1) {  // not found yet
      month = FindMonth(date_string);
      if (month == -1)
        return std::numeric_limits<double>::quiet_NaN();

      while (*date_string && *date_string != '-' && *date_string != ',' &&
             !IsASCIISpace(*date_string))
        date_string++;

      if (!*date_string)
        return std::numeric_limits<double>::quiet_NaN();

      // '-99 23:12:40 GMT'
      if (*date_string != '-' && *date_string != '/' && *date_string != ',' &&
          !IsASCIISpace(*date_string))
        return std::numeric_limits<double>::quiet_NaN();
      date_string++;
    }
  }

  if (month < 0 || month > 11)
    return std::numeric_limits<double>::quiet_NaN();

  // '99 23:12:40 GMT'
  if (year <= 0 && *date_string) {
    if (!ParseInt(date_string, &new_pos_str, 10, &year))
      return std::numeric_limits<double>::quiet_NaN();
  }

  // Don't fail if the time is missing.
  long hour = 0;
  long minute = 0;
  long second = 0;
  if (!*new_pos_str) {
    date_string = new_pos_str;
  } else {
    // ' 23:12:40 GMT'
    if (!(IsASCIISpace(*new_pos_str) || *new_pos_str == ',')) {
      if (*new_pos_str != ':')
        return std::numeric_limits<double>::quiet_NaN();
      // There was no year; the number was the hour.
      year = -1;
    } else {
      // in the normal case (we parsed the year), advance to the next number
      date_string = ++new_pos_str;
      SkipSpacesAndComments(date_string);
    }

    ParseLong(date_string, &new_pos_str, 10, &hour);
    // Do not check for errno here since we want to continue
    // even if errno was set becasue we are still looking
    // for the timezone!

    // Read a number? If not, this might be a timezone name.
    if (new_pos_str != date_string) {
      date_string = new_pos_str;

      if (hour < 0 || hour > 23)
        return std::numeric_limits<double>::quiet_NaN();

      if (!*date_string)
        return std::numeric_limits<double>::quiet_NaN();

      // ':12:40 GMT'
      if (*date_string++ != ':')
        return std::numeric_limits<double>::quiet_NaN();

      if (!ParseLong(date_string, &new_pos_str, 10, &minute))
        return std::numeric_limits<double>::quiet_NaN();
      date_string = new_pos_str;

      if (minute < 0 || minute > 59)
        return std::numeric_limits<double>::quiet_NaN();

      // ':40 GMT'
      if (*date_string && *date_string != ':' && !IsASCIISpace(*date_string))
        return std::numeric_limits<double>::quiet_NaN();

      // seconds are optional in rfc822 + rfc2822
      if (*date_string == ':') {
        date_string++;

        if (!ParseLong(date_string, &new_pos_str, 10, &second))
          return std::numeric_limits<double>::quiet_NaN();
        date_string = new_pos_str;

        if (second < 0 || second > 59)
          return std::numeric_limits<double>::quiet_NaN();
      }

      SkipSpacesAndComments(date_string);

      if (strncasecmp(date_string, "AM", 2) == 0) {
        if (hour > 12)
          return std::numeric_limits<double>::quiet_NaN();
        if (hour == 12)
          hour = 0;
        date_string += 2;
        SkipSpacesAndComments(date_string);
      } else if (strncasecmp(date_string, "PM", 2) == 0) {
        if (hour > 12)
          return std::numeric_limits<double>::quiet_NaN();
        if (hour != 12)
          hour += 12;
        date_string += 2;
        SkipSpacesAndComments(date_string);
      }
    }
  }

  // The year may be after the time but before the time zone.
  if (IsASCIIDigit(*date_string) && year == -1) {
    if (!ParseInt(date_string, &new_pos_str, 10, &year))
      return std::numeric_limits<double>::quiet_NaN();
    date_string = new_pos_str;
    SkipSpacesAndComments(date_string);
  }

  // Don't fail if the time zone is missing.
  // Some websites omit the time zone (4275206).
  if (*date_string) {
    if (strncasecmp(date_string, "GMT", 3) == 0 ||
        strncasecmp(date_string, "UTC", 3) == 0) {
      date_string += 3;
      have_tz = true;
    }

    if (*date_string == '+' || *date_string == '-') {
      int o;
      if (!ParseInt(date_string, &new_pos_str, 10, &o))
        return std::numeric_limits<double>::quiet_NaN();
      date_string = new_pos_str;

      if (o < -9959 || o > 9959)
        return std::numeric_limits<double>::quiet_NaN();

      int sgn = (o < 0) ? -1 : 1;
      o = abs(o);
      if (*date_string != ':') {
        if (o >= 24)
          offset = ((o / 100) * 60 + (o % 100)) * sgn;
        else
          offset = o * 60 * sgn;
      } else {         // GMT+05:00
        ++date_string;  // skip the ':'
        int o2;
        if (!ParseInt(date_string, &new_pos_str, 10, &o2))
          return std::numeric_limits<double>::quiet_NaN();
        date_string = new_pos_str;
        offset = (o * 60 + o2) * sgn;
      }
      have_tz = true;
    } else {
      for (size_t i = 0; i < arraysize(known_zones); ++i) {
        if (0 == strncasecmp(date_string, known_zones[i].tz_name,
                             strlen(known_zones[i].tz_name))) {
          offset = known_zones[i].tz_offset;
          date_string += strlen(known_zones[i].tz_name);
          have_tz = true;
          break;
        }
      }
    }
  }

  SkipSpacesAndComments(date_string);

  if (*date_string && year == -1) {
    if (!ParseInt(date_string, &new_pos_str, 10, &year))
      return std::numeric_limits<double>::quiet_NaN();
    date_string = new_pos_str;
    SkipSpacesAndComments(date_string);
  }

  // Trailing garbage
  if (*date_string)
    return std::numeric_limits<double>::quiet_NaN();

  // Y2K: Handle 2 digit years.
  if (year >= 0 && year < 100) {
    if (year < 50)
      year += 2000;
    else
      year += 1900;
  }

  return YmdhmsToSeconds(year, month + 1, day, hour, minute, second) *
         kMsPerSecond;
}

double ParseDateFromNullTerminatedCharacters(const char* date_string) {
  bool have_tz;
  int offset;
  double ms =
      ParseDateFromNullTerminatedCharacters(date_string, have_tz, offset);
  if (std::isnan(ms))
    return std::numeric_limits<double>::quiet_NaN();

  // fall back to local timezone
  if (!have_tz) {
    double utc_offset = CalculateUTCOffset();
    double dst_offset = CalculateDSTOffset(ms, utc_offset);
    offset = static_cast<int>((utc_offset + dst_offset) / kMsPerMinute);
  }
  return ms - (offset * kMsPerMinute);
}

// See http://tools.ietf.org/html/rfc2822#section-3.3 for more information.
String MakeRFC2822DateString(const Time date, int utc_offset) {
  Time::Exploded time_exploded;
  date.UTCExplode(&time_exploded);

  StringBuilder string_builder;
  string_builder.Append(kWeekdayName[time_exploded.day_of_week]);
  string_builder.Append(", ");
  string_builder.AppendNumber(time_exploded.day_of_month);
  string_builder.Append(' ');
  // |month| is 1-based in Exploded
  string_builder.Append(kMonthName[time_exploded.month - 1]);
  string_builder.Append(' ');
  string_builder.AppendNumber(time_exploded.year);
  string_builder.Append(' ');

  AppendTwoDigitNumber(string_builder, time_exploded.hour);
  string_builder.Append(':');
  AppendTwoDigitNumber(string_builder, time_exploded.minute);
  string_builder.Append(':');
  AppendTwoDigitNumber(string_builder, time_exploded.second);
  string_builder.Append(' ');

  string_builder.Append(utc_offset > 0 ? '+' : '-');
  int absolute_utc_offset = abs(utc_offset);
  AppendTwoDigitNumber(string_builder, absolute_utc_offset / 60);
  AppendTwoDigitNumber(string_builder, absolute_utc_offset % 60);

  return string_builder.ToString();
}

double ConvertToLocalTime(double ms) {
  double utc_offset = CalculateUTCOffset();
  double dst_offset = CalculateDSTOffset(ms, utc_offset);
  return (ms + utc_offset + dst_offset);
}

}  // namespace WTF

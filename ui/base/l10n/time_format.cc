// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/l10n/time_format.h"

#include <limits>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "third_party/icu/source/common/unicode/unistr.h"
#include "ui/base/l10n/formatter.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_export.h"
#include "ui/strings/grit/ui_strings.h"

using base::TimeDelta;
using ui::TimeFormat;

namespace ui {

UI_BASE_EXPORT base::LazyInstance<FormatterContainer>::Leaky g_container =
    LAZY_INSTANCE_INITIALIZER;

// static
base::string16 TimeFormat::Simple(TimeFormat::Format format,
                                  TimeFormat::Length length,
                                  const base::TimeDelta& delta) {
  return Detailed(format, length, 0, delta);
}

base::string16 TimeFormat::SimpleWithMonthAndYear(TimeFormat::Format format,
                                                  TimeFormat::Length length,
                                                  const base::TimeDelta& delta,
                                                  bool with_month_and_year) {
  return DetailedWithMonthAndYear(format, length, 0, delta,
                                  with_month_and_year);
}

// static
base::string16 TimeFormat::Detailed(TimeFormat::Format format,
                                    TimeFormat::Length length,
                                    int cutoff,
                                    const base::TimeDelta& delta) {
  return DetailedWithMonthAndYear(format, length, cutoff, delta, false);
}

base::string16 TimeFormat::DetailedWithMonthAndYear(
    TimeFormat::Format format,
    TimeFormat::Length length,
    int cutoff,
    const base::TimeDelta& delta,
    bool with_month_and_year) {
  if (delta < TimeDelta::FromSeconds(0)) {
    NOTREACHED() << "Negative duration";
    return base::string16();
  }

  // Negative cutoff: always use two-value format.
  if (cutoff < 0)
    cutoff = std::numeric_limits<int>::max();

  const TimeDelta one_minute(TimeDelta::FromMinutes(1));
  const TimeDelta one_hour(TimeDelta::FromHours(1));
  const TimeDelta one_day(TimeDelta::FromDays(1));

  // An average month is a twelfth of a year.
  const TimeDelta one_month(TimeDelta::FromDays(365) / 12);

  // Simplify one year to be 365 days.
  const TimeDelta one_year(TimeDelta::FromDays(365));

  const TimeDelta half_second(TimeDelta::FromMilliseconds(500));
  const TimeDelta half_minute(TimeDelta::FromSeconds(30));
  const TimeDelta half_hour(TimeDelta::FromMinutes(30));
  const TimeDelta half_day(TimeDelta::FromHours(12));

  // Rationale: Start by determining major (first) unit, then add minor (second)
  // unit if mandated by |cutoff|.
  icu::UnicodeString time_string;
  const Formatter* formatter = g_container.Get().Get(format, length);
  if (delta < one_minute - half_second) {
    // Anything up to 59.500 seconds is formatted as seconds.
    const int seconds = static_cast<int>((delta + half_second).InSeconds());
    formatter->Format(Formatter::UNIT_SEC, seconds, &time_string);

  } else if (delta < one_hour - (cutoff < 60 ? half_minute : half_second)) {
    // Anything up to 59.5 minutes (respectively 59:59.500 when |cutoff| permits
    // two-value output) is formatted as minutes (respectively minutes and
    // seconds).
    if (delta >= cutoff * one_minute - half_second) {
      const int minutes = (delta + half_minute).InMinutes();
      formatter->Format(Formatter::UNIT_MIN, minutes, &time_string);
    } else {
      const int minutes = (delta + half_second).InMinutes();
      const int seconds = static_cast<int>(
          (delta + half_second).InSeconds() % 60);
      formatter->Format(Formatter::TWO_UNITS_MIN_SEC,
                        minutes, seconds, &time_string);
    }

  } else if (delta < one_day - (cutoff < 24 ? half_hour : half_minute)) {
    // Anything up to 23.5 hours (respectively 23:59:30.000 when |cutoff|
    // permits two-value output) is formatted as hours (respectively hours and
    // minutes).
    if (delta >= cutoff * one_hour - half_minute) {
      const int hours = (delta + half_hour).InHours();
      formatter->Format(Formatter::UNIT_HOUR, hours, &time_string);
    } else {
      const int hours = (delta + half_minute).InHours();
      const int minutes = (delta + half_minute).InMinutes() % 60;
      formatter->Format(Formatter::TWO_UNITS_HOUR_MIN,
                        hours, minutes, &time_string);
    }
  } else if (!with_month_and_year || delta < one_month) {
    // Anything bigger is formatted as days (respectively days and hours).
    if (delta >= cutoff * one_day - half_hour) {
      const int days = (delta + half_day).InDays();
      formatter->Format(Formatter::UNIT_DAY, days, &time_string);
    } else {
      const int days = (delta + half_hour).InDays();
      const int hours = (delta + half_hour).InHours() % 24;
      formatter->Format(Formatter::TWO_UNITS_DAY_HOUR,
                        days, hours, &time_string);
    }
  } else if (delta < one_year) {
    DCHECK(with_month_and_year);
    int month = delta / one_month;
    DCHECK(month >= 1 && month <= 12);
    formatter->Format(Formatter::UNIT_MONTH, month, &time_string);
  } else {
    DCHECK(with_month_and_year);
    int year = delta / one_year;
    formatter->Format(Formatter::UNIT_YEAR, year, &time_string);
  }

  const int capacity = time_string.length() + 1;
  DCHECK_GT(capacity, 1);
  base::string16 result;
  UErrorCode error = U_ZERO_ERROR;
  time_string.extract(static_cast<UChar*>(base::WriteInto(&result, capacity)),
                      capacity, error);
  DCHECK(U_SUCCESS(error));
  return result;
}

// static
base::string16 TimeFormat::RelativeDate(
    const base::Time& time,
    const base::Time* optional_midnight_today) {
  base::Time midnight_today = optional_midnight_today
                                  ? *optional_midnight_today
                                  : base::Time::Now().LocalMidnight();
  TimeDelta day = TimeDelta::FromMicroseconds(base::Time::kMicrosecondsPerDay);
  base::Time tomorrow = midnight_today + day;
  base::Time yesterday = midnight_today - day;
  if (time >= tomorrow)
    return base::string16();
  else if (time >= midnight_today)
    return l10n_util::GetStringUTF16(IDS_PAST_TIME_TODAY);
  else if (time >= yesterday)
    return l10n_util::GetStringUTF16(IDS_PAST_TIME_YESTERDAY);
  return base::string16();
}

}  // namespace ui

/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/platform/text/locale_mac.h"

#import <Foundation/NSDateFormatter.h>
#import <Foundation/NSLocale.h>
#include <memory>
#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/platform/language.h"
#include "third_party/blink/renderer/platform/layout_test_support.h"
#include "third_party/blink/renderer/platform/wtf/date_math.h"
#include "third_party/blink/renderer/platform/wtf/retain_ptr.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

static inline String LanguageFromLocale(const String& locale) {
  String normalized_locale = locale;
  normalized_locale.Replace('-', '_');
  size_t separator_position = normalized_locale.find('_');
  if (separator_position == kNotFound)
    return normalized_locale;
  return normalized_locale.Left(separator_position);
}

static RetainPtr<NSLocale> DetermineLocale(const String& locale) {
  if (!LayoutTestSupport::IsRunningLayoutTest()) {
    RetainPtr<NSLocale> current_locale = [NSLocale currentLocale];
    String current_locale_language =
        LanguageFromLocale(String([current_locale.Get() localeIdentifier]));
    String locale_language = LanguageFromLocale(locale);
    if (DeprecatedEqualIgnoringCase(current_locale_language, locale_language))
      return current_locale;
  }
  // It seems initWithLocaleIdentifier accepts dash-separated locale identifier.
  return RetainPtr<NSLocale>(
      kAdoptNS, [[NSLocale alloc] initWithLocaleIdentifier:locale]);
}

std::unique_ptr<Locale> Locale::Create(const String& locale) {
  return LocaleMac::Create(DetermineLocale(locale).Get());
}

static RetainPtr<NSDateFormatter> CreateDateTimeFormatter(
    NSLocale* locale,
    NSCalendar* calendar,
    NSDateFormatterStyle date_style,
    NSDateFormatterStyle time_style) {
  NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
  [formatter setLocale:locale];
  [formatter setDateStyle:date_style];
  [formatter setTimeStyle:time_style];
  [formatter setTimeZone:[NSTimeZone timeZoneWithAbbreviation:@"UTC"]];
  [formatter setCalendar:calendar];
  return AdoptNS(formatter);
}

LocaleMac::LocaleMac(NSLocale* locale)
    : locale_(locale),
      gregorian_calendar_(
          kAdoptNS,
          [[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar]),
      did_initialize_number_data_(false) {
  NSArray* available_languages = [NSLocale ISOLanguageCodes];
  // NSLocale returns a lower case NSLocaleLanguageCode so we don't have care
  // about case.
  NSString* language = [locale_.Get() objectForKey:NSLocaleLanguageCode];
  if ([available_languages indexOfObject:language] == NSNotFound)
    locale_.AdoptNS(
        [[NSLocale alloc] initWithLocaleIdentifier:DefaultLanguage()]);
  [gregorian_calendar_.Get() setLocale:locale_.Get()];
}

LocaleMac::~LocaleMac() {}

std::unique_ptr<LocaleMac> LocaleMac::Create(const String& locale_identifier) {
  RetainPtr<NSLocale> locale =
      [[NSLocale alloc] initWithLocaleIdentifier:locale_identifier];
  return base::WrapUnique(new LocaleMac(locale.Get()));
}

std::unique_ptr<LocaleMac> LocaleMac::Create(NSLocale* locale) {
  return base::WrapUnique(new LocaleMac(locale));
}

RetainPtr<NSDateFormatter> LocaleMac::ShortDateFormatter() {
  return CreateDateTimeFormatter(locale_.Get(), gregorian_calendar_.Get(),
                                 NSDateFormatterShortStyle,
                                 NSDateFormatterNoStyle);
}

const Vector<String>& LocaleMac::MonthLabels() {
  if (!month_labels_.IsEmpty())
    return month_labels_;
  month_labels_.ReserveCapacity(12);
  NSArray* array = [ShortDateFormatter().Get() monthSymbols];
  if ([array count] == 12) {
    for (unsigned i = 0; i < 12; ++i)
      month_labels_.push_back(String([array objectAtIndex:i]));
    return month_labels_;
  }
  for (unsigned i = 0; i < arraysize(WTF::kMonthFullName); ++i)
    month_labels_.push_back(WTF::kMonthFullName[i]);
  return month_labels_;
}

const Vector<String>& LocaleMac::WeekDayShortLabels() {
  if (!week_day_short_labels_.IsEmpty())
    return week_day_short_labels_;
  week_day_short_labels_.ReserveCapacity(7);
  NSArray* array = [ShortDateFormatter().Get() shortWeekdaySymbols];
  if ([array count] == 7) {
    for (unsigned i = 0; i < 7; ++i)
      week_day_short_labels_.push_back(String([array objectAtIndex:i]));
    return week_day_short_labels_;
  }
  for (unsigned i = 0; i < arraysize(WTF::kWeekdayName); ++i) {
    // weekdayName starts with Monday.
    week_day_short_labels_.push_back(WTF::kWeekdayName[(i + 6) % 7]);
  }
  return week_day_short_labels_;
}

unsigned LocaleMac::FirstDayOfWeek() {
  // The document for NSCalendar - firstWeekday doesn't have an explanation of
  // firstWeekday value. We can guess it by the document of NSDateComponents -
  // weekDay, so it can be 1 through 7 and 1 is Sunday.
  return [gregorian_calendar_.Get() firstWeekday] - 1;
}

bool LocaleMac::IsRTL() {
  return NSLocaleLanguageDirectionRightToLeft ==
         [NSLocale characterDirectionForLanguage:
                       [NSLocale canonicalLanguageIdentifierFromString:
                                     [locale_.Get() localeIdentifier]]];
}

RetainPtr<NSDateFormatter> LocaleMac::TimeFormatter() {
  return CreateDateTimeFormatter(locale_.Get(), gregorian_calendar_.Get(),
                                 NSDateFormatterNoStyle,
                                 NSDateFormatterMediumStyle);
}

RetainPtr<NSDateFormatter> LocaleMac::ShortTimeFormatter() {
  return CreateDateTimeFormatter(locale_.Get(), gregorian_calendar_.Get(),
                                 NSDateFormatterNoStyle,
                                 NSDateFormatterShortStyle);
}

RetainPtr<NSDateFormatter> LocaleMac::DateTimeFormatterWithSeconds() {
  return CreateDateTimeFormatter(locale_.Get(), gregorian_calendar_.Get(),
                                 NSDateFormatterShortStyle,
                                 NSDateFormatterMediumStyle);
}

RetainPtr<NSDateFormatter> LocaleMac::DateTimeFormatterWithoutSeconds() {
  return CreateDateTimeFormatter(locale_.Get(), gregorian_calendar_.Get(),
                                 NSDateFormatterShortStyle,
                                 NSDateFormatterShortStyle);
}

String LocaleMac::DateFormat() {
  if (!date_format_.IsNull())
    return date_format_;
  date_format_ = [ShortDateFormatter().Get() dateFormat];
  return date_format_;
}

String LocaleMac::MonthFormat() {
  if (!month_format_.IsNull())
    return month_format_;
  // Gets a format for "MMMM" because Windows API always provides formats for
  // "MMMM" in some locales.
  month_format_ = [NSDateFormatter dateFormatFromTemplate:@"yyyyMMMM"
                                                  options:0
                                                   locale:locale_.Get()];
  return month_format_;
}

String LocaleMac::ShortMonthFormat() {
  if (!short_month_format_.IsNull())
    return short_month_format_;
  short_month_format_ = [NSDateFormatter dateFormatFromTemplate:@"yyyyMMM"
                                                        options:0
                                                         locale:locale_.Get()];
  return short_month_format_;
}

String LocaleMac::TimeFormat() {
  if (!time_format_with_seconds_.IsNull())
    return time_format_with_seconds_;
  time_format_with_seconds_ = [TimeFormatter().Get() dateFormat];
  return time_format_with_seconds_;
}

String LocaleMac::ShortTimeFormat() {
  if (!time_format_without_seconds_.IsNull())
    return time_format_without_seconds_;
  time_format_without_seconds_ = [ShortTimeFormatter().Get() dateFormat];
  return time_format_without_seconds_;
}

String LocaleMac::DateTimeFormatWithSeconds() {
  if (!date_time_format_with_seconds_.IsNull())
    return date_time_format_with_seconds_;
  date_time_format_with_seconds_ =
      [DateTimeFormatterWithSeconds().Get() dateFormat];
  return date_time_format_with_seconds_;
}

String LocaleMac::DateTimeFormatWithoutSeconds() {
  if (!date_time_format_without_seconds_.IsNull())
    return date_time_format_without_seconds_;
  date_time_format_without_seconds_ =
      [DateTimeFormatterWithoutSeconds().Get() dateFormat];
  return date_time_format_without_seconds_;
}

const Vector<String>& LocaleMac::ShortMonthLabels() {
  if (!short_month_labels_.IsEmpty())
    return short_month_labels_;
  short_month_labels_.ReserveCapacity(12);
  NSArray* array = [ShortDateFormatter().Get() shortMonthSymbols];
  if ([array count] == 12) {
    for (unsigned i = 0; i < 12; ++i)
      short_month_labels_.push_back([array objectAtIndex:i]);
    return short_month_labels_;
  }
  for (unsigned i = 0; i < arraysize(WTF::kMonthName); ++i)
    short_month_labels_.push_back(WTF::kMonthName[i]);
  return short_month_labels_;
}

const Vector<String>& LocaleMac::StandAloneMonthLabels() {
  if (!stand_alone_month_labels_.IsEmpty())
    return stand_alone_month_labels_;
  NSArray* array = [ShortDateFormatter().Get() standaloneMonthSymbols];
  if ([array count] == 12) {
    stand_alone_month_labels_.ReserveCapacity(12);
    for (unsigned i = 0; i < 12; ++i)
      stand_alone_month_labels_.push_back([array objectAtIndex:i]);
    return stand_alone_month_labels_;
  }
  stand_alone_month_labels_ = ShortMonthLabels();
  return stand_alone_month_labels_;
}

const Vector<String>& LocaleMac::ShortStandAloneMonthLabels() {
  if (!short_stand_alone_month_labels_.IsEmpty())
    return short_stand_alone_month_labels_;
  NSArray* array = [ShortDateFormatter().Get() shortStandaloneMonthSymbols];
  if ([array count] == 12) {
    short_stand_alone_month_labels_.ReserveCapacity(12);
    for (unsigned i = 0; i < 12; ++i)
      short_stand_alone_month_labels_.push_back([array objectAtIndex:i]);
    return short_stand_alone_month_labels_;
  }
  short_stand_alone_month_labels_ = ShortMonthLabels();
  return short_stand_alone_month_labels_;
}

const Vector<String>& LocaleMac::TimeAMPMLabels() {
  if (!time_ampm_labels_.IsEmpty())
    return time_ampm_labels_;
  time_ampm_labels_.ReserveCapacity(2);
  RetainPtr<NSDateFormatter> formatter = ShortTimeFormatter();
  time_ampm_labels_.push_back([formatter.Get() AMSymbol]);
  time_ampm_labels_.push_back([formatter.Get() PMSymbol]);
  return time_ampm_labels_;
}

void LocaleMac::InitializeLocaleData() {
  if (did_initialize_number_data_)
    return;
  did_initialize_number_data_ = true;

  RetainPtr<NSNumberFormatter> formatter(kAdoptNS,
                                         [[NSNumberFormatter alloc] init]);
  [formatter.Get() setLocale:locale_.Get()];
  [formatter.Get() setNumberStyle:NSNumberFormatterDecimalStyle];
  [formatter.Get() setUsesGroupingSeparator:NO];

  RetainPtr<NSNumber> sample_number(
      kAdoptNS, [[NSNumber alloc] initWithDouble:9876543210]);
  String nine_to_zero([formatter.Get() stringFromNumber:sample_number.Get()]);
  if (nine_to_zero.length() != 10)
    return;
  Vector<String, kDecimalSymbolsSize> symbols;
  for (unsigned i = 0; i < 10; ++i)
    symbols.push_back(nine_to_zero.Substring(9 - i, 1));
  DCHECK(symbols.size() == kDecimalSeparatorIndex);
  symbols.push_back([formatter.Get() decimalSeparator]);
  DCHECK(symbols.size() == kGroupSeparatorIndex);
  symbols.push_back([formatter.Get() groupingSeparator]);
  DCHECK(symbols.size() == kDecimalSymbolsSize);

  String positive_prefix([formatter.Get() positivePrefix]);
  String positive_suffix([formatter.Get() positiveSuffix]);
  String negative_prefix([formatter.Get() negativePrefix]);
  String negative_suffix([formatter.Get() negativeSuffix]);
  SetLocaleData(symbols, positive_prefix, positive_suffix, negative_prefix,
                negative_suffix);
}
}

/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/forms/date_time_field_elements.h"

#include "third_party/blink/renderer/core/html/forms/date_time_fields_state.h"
#include "third_party/blink/renderer/platform/date_components.h"
#include "third_party/blink/renderer/platform/text/platform_locale.h"
#include "third_party/blink/renderer/platform/wtf/date_math.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

using blink::WebLocalizedString;

static String QueryString(WebLocalizedString::Name name) {
  return Locale::DefaultLocale().QueryString(name);
}

DateTimeAMPMFieldElement::DateTimeAMPMFieldElement(
    Document& document,
    FieldOwner& field_owner,
    const Vector<String>& ampm_labels)
    : DateTimeSymbolicFieldElement(document, field_owner, ampm_labels, 0, 1) {}

DateTimeAMPMFieldElement* DateTimeAMPMFieldElement::Create(
    Document& document,
    FieldOwner& field_owner,
    const Vector<String>& ampm_labels) {
  DEFINE_STATIC_LOCAL(AtomicString, ampm_pseudo_id,
                      ("-webkit-datetime-edit-ampm-field"));
  DateTimeAMPMFieldElement* field =
      new DateTimeAMPMFieldElement(document, field_owner, ampm_labels);
  field->Initialize(ampm_pseudo_id,
                    QueryString(WebLocalizedString::kAXAMPMFieldText));
  return field;
}

void DateTimeAMPMFieldElement::PopulateDateTimeFieldsState(
    DateTimeFieldsState& date_time_fields_state) {
  if (HasValue())
    date_time_fields_state.SetAMPM(ValueAsInteger()
                                       ? DateTimeFieldsState::kAMPMValuePM
                                       : DateTimeFieldsState::kAMPMValueAM);
  else
    date_time_fields_state.SetAMPM(DateTimeFieldsState::kAMPMValueEmpty);
}

void DateTimeAMPMFieldElement::SetValueAsDate(const DateComponents& date) {
  SetValueAsInteger(date.Hour() >= 12 ? 1 : 0);
}

void DateTimeAMPMFieldElement::SetValueAsDateTimeFieldsState(
    const DateTimeFieldsState& date_time_fields_state) {
  if (date_time_fields_state.HasAMPM())
    SetValueAsInteger(date_time_fields_state.Ampm());
  else
    SetEmptyValue();
}

// ----------------------------

DateTimeDayFieldElement::DateTimeDayFieldElement(Document& document,
                                                 FieldOwner& field_owner,
                                                 const String& placeholder,
                                                 const Range& range)
    : DateTimeNumericFieldElement(document,
                                  field_owner,
                                  range,
                                  Range(1, 31),
                                  placeholder) {}

DateTimeDayFieldElement* DateTimeDayFieldElement::Create(
    Document& document,
    FieldOwner& field_owner,
    const String& placeholder,
    const Range& range) {
  DEFINE_STATIC_LOCAL(AtomicString, day_pseudo_id,
                      ("-webkit-datetime-edit-day-field"));
  DateTimeDayFieldElement* field = new DateTimeDayFieldElement(
      document, field_owner, placeholder.IsEmpty() ? "--" : placeholder, range);
  field->Initialize(day_pseudo_id,
                    QueryString(WebLocalizedString::kAXDayOfMonthFieldText));
  return field;
}

void DateTimeDayFieldElement::PopulateDateTimeFieldsState(
    DateTimeFieldsState& date_time_fields_state) {
  date_time_fields_state.SetDayOfMonth(
      HasValue() ? ValueAsInteger() : DateTimeFieldsState::kEmptyValue);
}

void DateTimeDayFieldElement::SetValueAsDate(const DateComponents& date) {
  SetValueAsInteger(date.MonthDay());
}

void DateTimeDayFieldElement::SetValueAsDateTimeFieldsState(
    const DateTimeFieldsState& date_time_fields_state) {
  if (!date_time_fields_state.HasDayOfMonth()) {
    SetEmptyValue();
    return;
  }

  const unsigned value = date_time_fields_state.DayOfMonth();
  if (GetRange().IsInRange(static_cast<int>(value))) {
    SetValueAsInteger(value);
    return;
  }

  SetEmptyValue();
}

// ----------------------------

DateTimeHourFieldElementBase::DateTimeHourFieldElementBase(
    Document& document,
    FieldOwner& field_owner,
    const Range& range,
    const Range& hard_limits,
    const Step& step)
    : DateTimeNumericFieldElement(document,
                                  field_owner,
                                  range,
                                  hard_limits,
                                  "--",
                                  step) {}

void DateTimeHourFieldElementBase::Initialize() {
  DEFINE_STATIC_LOCAL(AtomicString, hour_pseudo_id,
                      ("-webkit-datetime-edit-hour-field"));
  DateTimeNumericFieldElement::Initialize(
      hour_pseudo_id, QueryString(WebLocalizedString::kAXHourFieldText));
}

void DateTimeHourFieldElementBase::SetValueAsDate(const DateComponents& date) {
  SetValueAsInteger(date.Hour());
}

void DateTimeHourFieldElementBase::SetValueAsDateTimeFieldsState(
    const DateTimeFieldsState& date_time_fields_state) {
  if (!date_time_fields_state.HasHour()) {
    SetEmptyValue();
    return;
  }

  const int hour12 = date_time_fields_state.Hour();
  if (hour12 < 1 || hour12 > 12) {
    SetEmptyValue();
    return;
  }

  const int hour11 = hour12 == 12 ? 0 : hour12;
  const int hour23 =
      date_time_fields_state.Ampm() == DateTimeFieldsState::kAMPMValuePM
          ? hour11 + 12
          : hour11;
  SetValueAsInteger(hour23);
}
// ----------------------------

DateTimeHour11FieldElement::DateTimeHour11FieldElement(Document& document,
                                                       FieldOwner& field_owner,
                                                       const Range& range,
                                                       const Step& step)
    : DateTimeHourFieldElementBase(document,
                                   field_owner,
                                   range,
                                   Range(0, 11),
                                   step) {}

DateTimeHour11FieldElement* DateTimeHour11FieldElement::Create(
    Document& document,
    FieldOwner& field_owner,
    const Range& hour23_range,
    const Step& step) {
  DCHECK_GE(hour23_range.minimum, 0);
  DCHECK_LE(hour23_range.maximum, 23);
  DCHECK_LE(hour23_range.minimum, hour23_range.maximum);
  Range range(0, 11);
  if (hour23_range.maximum < 12) {
    range = hour23_range;
  } else if (hour23_range.minimum >= 12) {
    range.minimum = hour23_range.minimum - 12;
    range.maximum = hour23_range.maximum - 12;
  }

  DateTimeHour11FieldElement* field =
      new DateTimeHour11FieldElement(document, field_owner, range, step);
  field->Initialize();
  return field;
}

void DateTimeHour11FieldElement::PopulateDateTimeFieldsState(
    DateTimeFieldsState& date_time_fields_state) {
  if (!HasValue()) {
    date_time_fields_state.SetHour(DateTimeFieldsState::kEmptyValue);
    return;
  }
  const int value = ValueAsInteger();
  date_time_fields_state.SetHour(value ? value : 12);
}

void DateTimeHour11FieldElement::SetValueAsInteger(
    int value,
    EventBehavior event_behavior) {
  value = Range(0, 23).ClampValue(value) % 12;
  DateTimeNumericFieldElement::SetValueAsInteger(value, event_behavior);
}

// ----------------------------

DateTimeHour12FieldElement::DateTimeHour12FieldElement(Document& document,
                                                       FieldOwner& field_owner,
                                                       const Range& range,
                                                       const Step& step)
    : DateTimeHourFieldElementBase(document,
                                   field_owner,
                                   range,
                                   Range(1, 12),
                                   step) {}

DateTimeHour12FieldElement* DateTimeHour12FieldElement::Create(
    Document& document,
    FieldOwner& field_owner,
    const Range& hour23_range,
    const Step& step) {
  DCHECK_GE(hour23_range.minimum, 0);
  DCHECK_LE(hour23_range.maximum, 23);
  DCHECK_LE(hour23_range.minimum, hour23_range.maximum);
  Range range(1, 12);
  if (hour23_range.maximum < 12) {
    range = hour23_range;
  } else if (hour23_range.minimum >= 12) {
    range.minimum = hour23_range.minimum - 12;
    range.maximum = hour23_range.maximum - 12;
  }
  if (!range.minimum)
    range.minimum = 12;
  if (!range.maximum)
    range.maximum = 12;
  if (range.minimum > range.maximum) {
    range.minimum = 1;
    range.maximum = 12;
  }
  DateTimeHour12FieldElement* field =
      new DateTimeHour12FieldElement(document, field_owner, range, step);
  field->Initialize();
  return field;
}

void DateTimeHour12FieldElement::PopulateDateTimeFieldsState(
    DateTimeFieldsState& date_time_fields_state) {
  date_time_fields_state.SetHour(HasValue() ? ValueAsInteger()
                                            : DateTimeFieldsState::kEmptyValue);
}

void DateTimeHour12FieldElement::SetValueAsInteger(
    int value,
    EventBehavior event_behavior) {
  value = Range(0, 24).ClampValue(value) % 12;
  DateTimeNumericFieldElement::SetValueAsInteger(value ? value : 12,
                                                 event_behavior);
}

// ----------------------------

DateTimeHour23FieldElement::DateTimeHour23FieldElement(Document& document,
                                                       FieldOwner& field_owner,
                                                       const Range& range,
                                                       const Step& step)
    : DateTimeHourFieldElementBase(document,
                                   field_owner,
                                   range,
                                   Range(0, 23),
                                   step) {}

DateTimeHour23FieldElement* DateTimeHour23FieldElement::Create(
    Document& document,
    FieldOwner& field_owner,
    const Range& hour23_range,
    const Step& step) {
  DCHECK_GE(hour23_range.minimum, 0);
  DCHECK_LE(hour23_range.maximum, 23);
  DCHECK_LE(hour23_range.minimum, hour23_range.maximum);
  DateTimeHour23FieldElement* field =
      new DateTimeHour23FieldElement(document, field_owner, hour23_range, step);
  field->Initialize();
  return field;
}

void DateTimeHour23FieldElement::PopulateDateTimeFieldsState(
    DateTimeFieldsState& date_time_fields_state) {
  if (!HasValue()) {
    date_time_fields_state.SetHour(DateTimeFieldsState::kEmptyValue);
    return;
  }

  const int value = ValueAsInteger();

  date_time_fields_state.SetHour(value % 12 ? value % 12 : 12);
  date_time_fields_state.SetAMPM(value >= 12
                                     ? DateTimeFieldsState::kAMPMValuePM
                                     : DateTimeFieldsState::kAMPMValueAM);
}

void DateTimeHour23FieldElement::SetValueAsInteger(
    int value,
    EventBehavior event_behavior) {
  value = Range(0, 23).ClampValue(value);
  DateTimeNumericFieldElement::SetValueAsInteger(value, event_behavior);
}

// ----------------------------

DateTimeHour24FieldElement::DateTimeHour24FieldElement(Document& document,
                                                       FieldOwner& field_owner,
                                                       const Range& range,
                                                       const Step& step)
    : DateTimeHourFieldElementBase(document,
                                   field_owner,
                                   range,
                                   Range(1, 24),
                                   step) {}

DateTimeHour24FieldElement* DateTimeHour24FieldElement::Create(
    Document& document,
    FieldOwner& field_owner,
    const Range& hour23_range,
    const Step& step) {
  DCHECK_GE(hour23_range.minimum, 0);
  DCHECK_LE(hour23_range.maximum, 23);
  DCHECK_LE(hour23_range.minimum, hour23_range.maximum);
  Range range(hour23_range.minimum ? hour23_range.minimum : 24,
              hour23_range.maximum ? hour23_range.maximum : 24);
  if (range.minimum > range.maximum) {
    range.minimum = 1;
    range.maximum = 24;
  }

  DateTimeHour24FieldElement* field =
      new DateTimeHour24FieldElement(document, field_owner, range, step);
  field->Initialize();
  return field;
}

void DateTimeHour24FieldElement::PopulateDateTimeFieldsState(
    DateTimeFieldsState& date_time_fields_state) {
  if (!HasValue()) {
    date_time_fields_state.SetHour(DateTimeFieldsState::kEmptyValue);
    return;
  }

  const int value = ValueAsInteger();

  if (value == 24) {
    date_time_fields_state.SetHour(12);
    date_time_fields_state.SetAMPM(DateTimeFieldsState::kAMPMValueAM);
  } else {
    date_time_fields_state.SetHour(value == 12 ? 12 : value % 12);
    date_time_fields_state.SetAMPM(value >= 12
                                       ? DateTimeFieldsState::kAMPMValuePM
                                       : DateTimeFieldsState::kAMPMValueAM);
  }
}

void DateTimeHour24FieldElement::SetValueAsInteger(
    int value,
    EventBehavior event_behavior) {
  value = Range(0, 24).ClampValue(value);
  DateTimeNumericFieldElement::SetValueAsInteger(value ? value : 24,
                                                 event_behavior);
}

// ----------------------------

DateTimeMillisecondFieldElement::DateTimeMillisecondFieldElement(
    Document& document,
    FieldOwner& field_owner,
    const Range& range,
    const Step& step)
    : DateTimeNumericFieldElement(document,
                                  field_owner,
                                  range,
                                  Range(0, 999),
                                  "---",
                                  step) {}

DateTimeMillisecondFieldElement* DateTimeMillisecondFieldElement::Create(
    Document& document,
    FieldOwner& field_owner,
    const Range& range,
    const Step& step) {
  DEFINE_STATIC_LOCAL(AtomicString, millisecond_pseudo_id,
                      ("-webkit-datetime-edit-millisecond-field"));
  DateTimeMillisecondFieldElement* field =
      new DateTimeMillisecondFieldElement(document, field_owner, range, step);
  field->Initialize(millisecond_pseudo_id,
                    QueryString(WebLocalizedString::kAXMillisecondFieldText));
  return field;
}

void DateTimeMillisecondFieldElement::PopulateDateTimeFieldsState(
    DateTimeFieldsState& date_time_fields_state) {
  date_time_fields_state.SetMillisecond(
      HasValue() ? ValueAsInteger() : DateTimeFieldsState::kEmptyValue);
}

void DateTimeMillisecondFieldElement::SetValueAsDate(
    const DateComponents& date) {
  SetValueAsInteger(date.Millisecond());
}

void DateTimeMillisecondFieldElement::SetValueAsDateTimeFieldsState(
    const DateTimeFieldsState& date_time_fields_state) {
  if (!date_time_fields_state.HasMillisecond()) {
    SetEmptyValue();
    return;
  }

  const unsigned value = date_time_fields_state.Millisecond();
  if (value > static_cast<unsigned>(Maximum())) {
    SetEmptyValue();
    return;
  }

  SetValueAsInteger(value);
}

// ----------------------------

DateTimeMinuteFieldElement::DateTimeMinuteFieldElement(Document& document,
                                                       FieldOwner& field_owner,
                                                       const Range& range,
                                                       const Step& step)
    : DateTimeNumericFieldElement(document,
                                  field_owner,
                                  range,
                                  Range(0, 59),
                                  "--",
                                  step) {}

DateTimeMinuteFieldElement* DateTimeMinuteFieldElement::Create(
    Document& document,
    FieldOwner& field_owner,
    const Range& range,
    const Step& step) {
  DEFINE_STATIC_LOCAL(AtomicString, minute_pseudo_id,
                      ("-webkit-datetime-edit-minute-field"));
  DateTimeMinuteFieldElement* field =
      new DateTimeMinuteFieldElement(document, field_owner, range, step);
  field->Initialize(minute_pseudo_id,
                    QueryString(WebLocalizedString::kAXMinuteFieldText));
  return field;
}

void DateTimeMinuteFieldElement::PopulateDateTimeFieldsState(
    DateTimeFieldsState& date_time_fields_state) {
  date_time_fields_state.SetMinute(
      HasValue() ? ValueAsInteger() : DateTimeFieldsState::kEmptyValue);
}

void DateTimeMinuteFieldElement::SetValueAsDate(const DateComponents& date) {
  SetValueAsInteger(date.Minute());
}

void DateTimeMinuteFieldElement::SetValueAsDateTimeFieldsState(
    const DateTimeFieldsState& date_time_fields_state) {
  if (!date_time_fields_state.HasMinute()) {
    SetEmptyValue();
    return;
  }

  const unsigned value = date_time_fields_state.Minute();
  if (value > static_cast<unsigned>(Maximum())) {
    SetEmptyValue();
    return;
  }

  SetValueAsInteger(value);
}

// ----------------------------

DateTimeMonthFieldElement::DateTimeMonthFieldElement(Document& document,
                                                     FieldOwner& field_owner,
                                                     const String& placeholder,
                                                     const Range& range)
    : DateTimeNumericFieldElement(document,
                                  field_owner,
                                  range,
                                  Range(1, 12),
                                  placeholder) {}

DateTimeMonthFieldElement* DateTimeMonthFieldElement::Create(
    Document& document,
    FieldOwner& field_owner,
    const String& placeholder,
    const Range& range) {
  DEFINE_STATIC_LOCAL(AtomicString, month_pseudo_id,
                      ("-webkit-datetime-edit-month-field"));
  DateTimeMonthFieldElement* field = new DateTimeMonthFieldElement(
      document, field_owner, placeholder.IsEmpty() ? "--" : placeholder, range);
  field->Initialize(month_pseudo_id,
                    QueryString(WebLocalizedString::kAXMonthFieldText));
  return field;
}

void DateTimeMonthFieldElement::PopulateDateTimeFieldsState(
    DateTimeFieldsState& date_time_fields_state) {
  date_time_fields_state.SetMonth(
      HasValue() ? ValueAsInteger() : DateTimeFieldsState::kEmptyValue);
}

void DateTimeMonthFieldElement::SetValueAsDate(const DateComponents& date) {
  SetValueAsInteger(date.Month() + 1);
}

void DateTimeMonthFieldElement::SetValueAsDateTimeFieldsState(
    const DateTimeFieldsState& date_time_fields_state) {
  if (!date_time_fields_state.HasMonth()) {
    SetEmptyValue();
    return;
  }

  const unsigned value = date_time_fields_state.Month();
  if (GetRange().IsInRange(static_cast<int>(value))) {
    SetValueAsInteger(value);
    return;
  }

  SetEmptyValue();
}

// ----------------------------

DateTimeSecondFieldElement::DateTimeSecondFieldElement(Document& document,
                                                       FieldOwner& field_owner,
                                                       const Range& range,
                                                       const Step& step)
    : DateTimeNumericFieldElement(document,
                                  field_owner,
                                  range,
                                  Range(0, 59),
                                  "--",
                                  step) {}

DateTimeSecondFieldElement* DateTimeSecondFieldElement::Create(
    Document& document,
    FieldOwner& field_owner,
    const Range& range,
    const Step& step) {
  DEFINE_STATIC_LOCAL(AtomicString, second_pseudo_id,
                      ("-webkit-datetime-edit-second-field"));
  DateTimeSecondFieldElement* field =
      new DateTimeSecondFieldElement(document, field_owner, range, step);
  field->Initialize(second_pseudo_id,
                    QueryString(WebLocalizedString::kAXSecondFieldText));
  return field;
}

void DateTimeSecondFieldElement::PopulateDateTimeFieldsState(
    DateTimeFieldsState& date_time_fields_state) {
  date_time_fields_state.SetSecond(
      HasValue() ? ValueAsInteger() : DateTimeFieldsState::kEmptyValue);
}

void DateTimeSecondFieldElement::SetValueAsDate(const DateComponents& date) {
  SetValueAsInteger(date.Second());
}

void DateTimeSecondFieldElement::SetValueAsDateTimeFieldsState(
    const DateTimeFieldsState& date_time_fields_state) {
  if (!date_time_fields_state.HasSecond()) {
    SetEmptyValue();
    return;
  }

  const unsigned value = date_time_fields_state.Second();
  if (value > static_cast<unsigned>(Maximum())) {
    SetEmptyValue();
    return;
  }

  SetValueAsInteger(value);
}

// ----------------------------

DateTimeSymbolicMonthFieldElement::DateTimeSymbolicMonthFieldElement(
    Document& document,
    FieldOwner& field_owner,
    const Vector<String>& labels,
    int minimum,
    int maximum)
    : DateTimeSymbolicFieldElement(document,
                                   field_owner,
                                   labels,
                                   minimum,
                                   maximum) {}

DateTimeSymbolicMonthFieldElement* DateTimeSymbolicMonthFieldElement::Create(
    Document& document,
    FieldOwner& field_owner,
    const Vector<String>& labels,
    int minimum,
    int maximum) {
  DEFINE_STATIC_LOCAL(AtomicString, month_pseudo_id,
                      ("-webkit-datetime-edit-month-field"));
  DateTimeSymbolicMonthFieldElement* field =
      new DateTimeSymbolicMonthFieldElement(document, field_owner, labels,
                                            minimum, maximum);
  field->Initialize(month_pseudo_id,
                    QueryString(WebLocalizedString::kAXMonthFieldText));
  return field;
}

void DateTimeSymbolicMonthFieldElement::PopulateDateTimeFieldsState(
    DateTimeFieldsState& date_time_fields_state) {
  if (!HasValue())
    date_time_fields_state.SetMonth(DateTimeFieldsState::kEmptyValue);
  DCHECK_LT(ValueAsInteger(), static_cast<int>(SymbolsSize()));
  date_time_fields_state.SetMonth(ValueAsInteger() + 1);
}

void DateTimeSymbolicMonthFieldElement::SetValueAsDate(
    const DateComponents& date) {
  SetValueAsInteger(date.Month());
}

void DateTimeSymbolicMonthFieldElement::SetValueAsDateTimeFieldsState(
    const DateTimeFieldsState& date_time_fields_state) {
  if (!date_time_fields_state.HasMonth()) {
    SetEmptyValue();
    return;
  }

  const unsigned value = date_time_fields_state.Month() - 1;
  if (value >= SymbolsSize()) {
    SetEmptyValue();
    return;
  }

  SetValueAsInteger(value);
}

// ----------------------------

DateTimeWeekFieldElement::DateTimeWeekFieldElement(Document& document,
                                                   FieldOwner& field_owner,
                                                   const Range& range)
    : DateTimeNumericFieldElement(document,
                                  field_owner,
                                  range,
                                  Range(DateComponents::kMinimumWeekNumber,
                                        DateComponents::kMaximumWeekNumber),
                                  "--") {}

DateTimeWeekFieldElement* DateTimeWeekFieldElement::Create(
    Document& document,
    FieldOwner& field_owner,
    const Range& range) {
  DEFINE_STATIC_LOCAL(AtomicString, week_pseudo_id,
                      ("-webkit-datetime-edit-week-field"));
  DateTimeWeekFieldElement* field =
      new DateTimeWeekFieldElement(document, field_owner, range);
  field->Initialize(week_pseudo_id,
                    QueryString(WebLocalizedString::kAXWeekOfYearFieldText));
  return field;
}

void DateTimeWeekFieldElement::PopulateDateTimeFieldsState(
    DateTimeFieldsState& date_time_fields_state) {
  date_time_fields_state.SetWeekOfYear(
      HasValue() ? ValueAsInteger() : DateTimeFieldsState::kEmptyValue);
}

void DateTimeWeekFieldElement::SetValueAsDate(const DateComponents& date) {
  SetValueAsInteger(date.Week());
}

void DateTimeWeekFieldElement::SetValueAsDateTimeFieldsState(
    const DateTimeFieldsState& date_time_fields_state) {
  if (!date_time_fields_state.HasWeekOfYear()) {
    SetEmptyValue();
    return;
  }

  const unsigned value = date_time_fields_state.WeekOfYear();
  if (GetRange().IsInRange(static_cast<int>(value))) {
    SetValueAsInteger(value);
    return;
  }

  SetEmptyValue();
}

// ----------------------------

DateTimeYearFieldElement::DateTimeYearFieldElement(
    Document& document,
    FieldOwner& field_owner,
    const DateTimeYearFieldElement::Parameters& parameters)
    : DateTimeNumericFieldElement(
          document,
          field_owner,
          Range(parameters.minimum_year, parameters.maximum_year),
          Range(DateComponents::MinimumYear(), DateComponents::MaximumYear()),
          parameters.placeholder.IsEmpty() ? "----" : parameters.placeholder),
      min_is_specified_(parameters.min_is_specified),
      max_is_specified_(parameters.max_is_specified) {
  DCHECK_GE(parameters.minimum_year, DateComponents::MinimumYear());
  DCHECK_LE(parameters.maximum_year, DateComponents::MaximumYear());
}

DateTimeYearFieldElement* DateTimeYearFieldElement::Create(
    Document& document,
    FieldOwner& field_owner,
    const DateTimeYearFieldElement::Parameters& parameters) {
  DEFINE_STATIC_LOCAL(AtomicString, year_pseudo_id,
                      ("-webkit-datetime-edit-year-field"));
  DateTimeYearFieldElement* field =
      new DateTimeYearFieldElement(document, field_owner, parameters);
  field->Initialize(year_pseudo_id,
                    QueryString(WebLocalizedString::kAXYearFieldText));
  return field;
}

static int CurrentFullYear() {
  DateComponents date;
  date.SetMillisecondsSinceEpochForMonth(ConvertToLocalTime(CurrentTimeMS()));
  return date.FullYear();
}

int DateTimeYearFieldElement::DefaultValueForStepDown() const {
  return max_is_specified_
             ? DateTimeNumericFieldElement::DefaultValueForStepDown()
             : CurrentFullYear();
}

int DateTimeYearFieldElement::DefaultValueForStepUp() const {
  return min_is_specified_
             ? DateTimeNumericFieldElement::DefaultValueForStepUp()
             : CurrentFullYear();
}

void DateTimeYearFieldElement::PopulateDateTimeFieldsState(
    DateTimeFieldsState& date_time_fields_state) {
  date_time_fields_state.SetYear(HasValue() ? ValueAsInteger()
                                            : DateTimeFieldsState::kEmptyValue);
}

void DateTimeYearFieldElement::SetValueAsDate(const DateComponents& date) {
  SetValueAsInteger(date.FullYear());
}

void DateTimeYearFieldElement::SetValueAsDateTimeFieldsState(
    const DateTimeFieldsState& date_time_fields_state) {
  if (!date_time_fields_state.HasYear()) {
    SetEmptyValue();
    return;
  }

  const unsigned value = date_time_fields_state.Year();
  if (GetRange().IsInRange(static_cast<int>(value))) {
    SetValueAsInteger(value);
    return;
  }

  SetEmptyValue();
}

}  // namespace blink

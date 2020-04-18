// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/experiment_labels.h"

#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"

namespace installer {

namespace {

constexpr base::StringPiece16::value_type kNameValueSeparator = L'=';
constexpr base::StringPiece16::value_type kValueExpirationSeparator = L'|';
constexpr base::StringPiece16::value_type kLabelSeparator = L';';

// Returns a vector of string pieces, one for each "name=value|expiration"
// group in |value|.
std::vector<base::StringPiece16> Parse(base::StringPiece16 value) {
  static constexpr base::char16 kLabelSeparatorString[] = {kLabelSeparator,
                                                           L'\0'};
  return base::SplitStringPiece(value, kLabelSeparatorString,
                                base::TRIM_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);
}

// Returns an abbreviated day name for a zero-based |day_of_week|.
const wchar_t* AbbreviatedDayOfWeek(int day_of_week) {
  // Matches the abbreviated day names from the ICU "en" locale.
  static constexpr const wchar_t* kDays[] = {L"Sun", L"Mon", L"Tue", L"Wed",
                                             L"Thu", L"Fri", L"Sat"};
  return kDays[day_of_week];
}

// Returns an abbreviated month name for a one-based |month|.
const wchar_t* AbbreviatedMonth(int month) {
  // Matches the abbreviated month names from the ICU "en" locale.
  static constexpr const wchar_t* kMonths[] = {L"Jan", L"Feb", L"Mar", L"Apr",
                                               L"May", L"Jun", L"Jul", L"Aug",
                                               L"Sep", L"Oct", L"Nov", L"Dec"};
  return kMonths[month - 1];
}

// Returns a formatted string given a date that is compatible with Omaha (see
// https://github.com/google/omaha/blob/master/omaha/base/time.cc#L132).
base::string16 FormatDate(base::Time date) {
  base::Time::Exploded exploded_time;
  date.UTCExplode(&exploded_time);

  // "Fri, 14 Aug 2015 16:13:03 GMT"
  return base::StringPrintf(L"%ls, %02d %ls %04d %02d:%02d:%02d GMT",
                            AbbreviatedDayOfWeek(exploded_time.day_of_week),
                            exploded_time.day_of_month,
                            AbbreviatedMonth(exploded_time.month),
                            exploded_time.year, exploded_time.hour,
                            exploded_time.minute, exploded_time.second);
}

// Appends "label_name=label_value|expiration" to |label|.
void AppendLabel(base::StringPiece16 label_name,
                 base::StringPiece16 label_value,
                 base::Time expiration,
                 base::string16* label) {
  // 29 characters for the expiration date plus the two separators makes 31.
  label->reserve(label->size() + label_name.size() + label_value.size() + 31);
  label_name.AppendToString(label);
  label->push_back(kNameValueSeparator);
  label_value.AppendToString(label);
  label->push_back(kValueExpirationSeparator);
  label->append(FormatDate(expiration));
}

}  // namespace

ExperimentLabels::ExperimentLabels(const base::string16& value)
    : value_(value) {}

base::StringPiece16 ExperimentLabels::GetValueForLabel(
    base::StringPiece16 label_name) const {
  DCHECK(!label_name.empty());

  return FindLabel(label_name).second;
}

void ExperimentLabels::SetValueForLabel(base::StringPiece16 label_name,
                                        base::StringPiece16 label_value,
                                        base::TimeDelta lifetime) {
  DCHECK(!label_name.empty());
  DCHECK(!label_value.empty());
  DCHECK(!lifetime.is_zero());

  SetValueForLabel(label_name, label_value, base::Time::Now() + lifetime);
}

void ExperimentLabels::SetValueForLabel(base::StringPiece16 label_name,
                                        base::StringPiece16 label_value,
                                        base::Time expiration) {
  DCHECK(!label_name.empty());
  DCHECK(!label_value.empty());

  LabelAndValue label_and_value = FindLabel(label_name);
  if (label_and_value.first.empty()) {
    // This label doesn't already exist -- append it to the raw value.
    if (!value_.empty())
      value_.push_back(kLabelSeparator);
    AppendLabel(label_name, label_value, expiration, &value_);
  } else {
    // Replace the existing value and expiration.
    // Get the stuff before the old label.
    base::string16 new_label(value_, 0,
                             label_and_value.first.data() - value_.data());
    // Append the new label.
    AppendLabel(label_name, label_value, expiration, &new_label);
    // Find the stuff after the old label and append it.
    size_t next_separator = value_.find(
        kLabelSeparator,
        (label_and_value.second.data() + label_and_value.second.size()) -
            value_.data());
    if (next_separator != base::string16::npos)
      new_label.append(value_, next_separator, base::string16::npos);
    // Presto.
    new_label.swap(value_);
  }
}

ExperimentLabels::LabelAndValue ExperimentLabels::FindLabel(
    base::StringPiece16 label_name) const {
  DCHECK(!label_name.empty());

  std::vector<base::StringPiece16> labels = Parse(value_);
  for (const auto& label : labels) {
    if (label.size() < label_name.size() + 2 ||
        !label.starts_with(label_name) ||
        label[label_name.size()] != kNameValueSeparator) {
      continue;
    }
    size_t value_start = label_name.size() + 1;
    size_t value_end = label.find(kValueExpirationSeparator, value_start);
    if (value_end == base::StringPiece16::npos)
      break;
    return std::make_pair(label,
                          label.substr(value_start, value_end - value_start));
  }
  return LabelAndValue();
}

}  // namespace installer

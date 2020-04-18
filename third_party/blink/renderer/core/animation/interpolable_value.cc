// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/interpolable_value.h"

#include <memory>

namespace blink {

bool InterpolableNumber::Equals(const InterpolableValue& other) const {
  return value_ == ToInterpolableNumber(other).value_;
}

bool InterpolableList::Equals(const InterpolableValue& other) const {
  const InterpolableList& other_list = ToInterpolableList(other);
  if (length() != other_list.length())
    return false;
  for (size_t i = 0; i < length(); i++) {
    if (!values_[i]->Equals(*other_list.values_[i]))
      return false;
  }
  return true;
}

void InterpolableNumber::Interpolate(const InterpolableValue& to,
                                     const double progress,
                                     InterpolableValue& result) const {
  const InterpolableNumber& to_number = ToInterpolableNumber(to);
  InterpolableNumber& result_number = ToInterpolableNumber(result);

  if (progress == 0 || value_ == to_number.value_)
    result_number.value_ = value_;
  else if (progress == 1)
    result_number.value_ = to_number.value_;
  else
    result_number.value_ =
        value_ * (1 - progress) + to_number.value_ * progress;
}

void InterpolableList::Interpolate(const InterpolableValue& to,
                                   const double progress,
                                   InterpolableValue& result) const {
  const InterpolableList& to_list = ToInterpolableList(to);
  InterpolableList& result_list = ToInterpolableList(result);

  DCHECK_EQ(to_list.length(), length());
  DCHECK_EQ(result_list.length(), length());

  for (size_t i = 0; i < length(); i++) {
    DCHECK(values_[i]);
    DCHECK(to_list.values_[i]);
    values_[i]->Interpolate(*(to_list.values_[i]), progress,
                            *(result_list.values_[i]));
  }
}

std::unique_ptr<InterpolableValue> InterpolableList::CloneAndZero() const {
  std::unique_ptr<InterpolableList> result = InterpolableList::Create(length());
  for (size_t i = 0; i < length(); i++)
    result->Set(i, values_[i]->CloneAndZero());
  return std::move(result);
}

void InterpolableNumber::Scale(double scale) {
  value_ = value_ * scale;
}

void InterpolableList::Scale(double scale) {
  for (size_t i = 0; i < length(); i++)
    values_[i]->Scale(scale);
}

void InterpolableNumber::ScaleAndAdd(double scale,
                                     const InterpolableValue& other) {
  value_ = value_ * scale + ToInterpolableNumber(other).value_;
}

void InterpolableList::ScaleAndAdd(double scale,
                                   const InterpolableValue& other) {
  const InterpolableList& other_list = ToInterpolableList(other);
  DCHECK_EQ(other_list.length(), length());
  for (size_t i = 0; i < length(); i++)
    values_[i]->ScaleAndAdd(scale, *other_list.values_[i]);
}

}  // namespace blink

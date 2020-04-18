// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/list_interpolation_functions.h"

#include <memory>
#include "third_party/blink/renderer/core/animation/underlying_value_owner.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"

namespace blink {

DEFINE_NON_INTERPOLABLE_VALUE_TYPE(NonInterpolableList);

const size_t kRepeatableListMaxLength = 1000;

bool ListInterpolationFunctions::EqualValues(
    const InterpolationValue& a,
    const InterpolationValue& b,
    EqualNonInterpolableValuesCallback equal_non_interpolable_values) {
  if (!a && !b)
    return true;

  if (!a || !b)
    return false;

  const InterpolableList& interpolable_list_a =
      ToInterpolableList(*a.interpolable_value);
  const InterpolableList& interpolable_list_b =
      ToInterpolableList(*b.interpolable_value);

  if (interpolable_list_a.length() != interpolable_list_b.length())
    return false;

  size_t length = interpolable_list_a.length();
  if (length == 0)
    return true;

  const NonInterpolableList& non_interpolable_list_a =
      ToNonInterpolableList(*a.non_interpolable_value);
  const NonInterpolableList& non_interpolable_list_b =
      ToNonInterpolableList(*b.non_interpolable_value);

  for (size_t i = 0; i < length; i++) {
    if (!equal_non_interpolable_values(non_interpolable_list_a.Get(i),
                                       non_interpolable_list_b.Get(i)))
      return false;
  }
  return true;
}

static size_t MatchLengths(size_t start_length,
                           size_t end_length,
                           ListInterpolationFunctions::LengthMatchingStrategy
                               length_matching_strategy) {
  if (length_matching_strategy ==
      ListInterpolationFunctions::LengthMatchingStrategy::
          kLowestCommonMultiple) {
    // Combining the length expansion of lowestCommonMultiple with CSS
    // transitions has the potential to create pathological cases where this
    // algorithm compounds upon itself as the user starts transitions on already
    // animating values multiple times. This maximum limit is to avoid locking
    // up users' systems with memory consumption in the event that this occurs.
    // See crbug.com/739197 for more context.
    return std::min(kRepeatableListMaxLength,
                    lowestCommonMultiple(start_length, end_length));
  }
  DCHECK_EQ(length_matching_strategy,
            ListInterpolationFunctions::LengthMatchingStrategy::kPadToLargest);
  return std::max(start_length, end_length);
}

PairwiseInterpolationValue ListInterpolationFunctions::MaybeMergeSingles(
    InterpolationValue&& start,
    InterpolationValue&& end,
    LengthMatchingStrategy length_matching_strategy,
    MergeSingleItemConversionsCallback merge_single_item_conversions) {
  const size_t start_length =
      ToInterpolableList(*start.interpolable_value).length();
  const size_t end_length =
      ToInterpolableList(*end.interpolable_value).length();

  if (start_length == 0 && end_length == 0) {
    return PairwiseInterpolationValue(std::move(start.interpolable_value),
                                      std::move(end.interpolable_value),
                                      nullptr);
  }

  if (start_length == 0) {
    std::unique_ptr<InterpolableValue> start_interpolable_value =
        end.interpolable_value->CloneAndZero();
    return PairwiseInterpolationValue(std::move(start_interpolable_value),
                                      std::move(end.interpolable_value),
                                      std::move(end.non_interpolable_value));
  }

  if (end_length == 0) {
    std::unique_ptr<InterpolableValue> end_interpolable_value =
        start.interpolable_value->CloneAndZero();
    return PairwiseInterpolationValue(std::move(start.interpolable_value),
                                      std::move(end_interpolable_value),
                                      std::move(start.non_interpolable_value));
  }

  const size_t final_length =
      MatchLengths(start_length, end_length, length_matching_strategy);
  std::unique_ptr<InterpolableList> result_start_interpolable_list =
      InterpolableList::Create(final_length);
  std::unique_ptr<InterpolableList> result_end_interpolable_list =
      InterpolableList::Create(final_length);
  Vector<scoped_refptr<NonInterpolableValue>> result_non_interpolable_values(
      final_length);

  InterpolableList& start_interpolable_list =
      ToInterpolableList(*start.interpolable_value);
  InterpolableList& end_interpolable_list =
      ToInterpolableList(*end.interpolable_value);
  NonInterpolableList& start_non_interpolable_list =
      ToNonInterpolableList(*start.non_interpolable_value);
  NonInterpolableList& end_non_interpolable_list =
      ToNonInterpolableList(*end.non_interpolable_value);

  for (size_t i = 0; i < final_length; i++) {
    PairwiseInterpolationValue result = nullptr;
    if (length_matching_strategy ==
            LengthMatchingStrategy::kLowestCommonMultiple ||
        (i < start_length && i < end_length)) {
      InterpolationValue start(
          start_interpolable_list.Get(i % start_length)->Clone(),
          start_non_interpolable_list.Get(i % start_length));
      InterpolationValue end(end_interpolable_list.Get(i % end_length)->Clone(),
                             end_non_interpolable_list.Get(i % end_length));
      PairwiseInterpolationValue result =
          merge_single_item_conversions(std::move(start), std::move(end));
      if (!result)
        return nullptr;
      result_start_interpolable_list->Set(
          i, std::move(result.start_interpolable_value));
      result_end_interpolable_list->Set(
          i, std::move(result.end_interpolable_value));
      result_non_interpolable_values[i] =
          std::move(result.non_interpolable_value);
    } else {
      DCHECK_EQ(length_matching_strategy,
                LengthMatchingStrategy::kPadToLargest);
      if (i < start_length) {
        result_start_interpolable_list->Set(
            i, start_interpolable_list.Get(i)->Clone());
        result_end_interpolable_list->Set(
            i, start_interpolable_list.Get(i)->CloneAndZero());
        result_non_interpolable_values[i] = start_non_interpolable_list.Get(i);
      } else {
        DCHECK_LT(i, end_length);
        result_start_interpolable_list->Set(
            i, end_interpolable_list.Get(i)->CloneAndZero());
        result_end_interpolable_list->Set(
            i, end_interpolable_list.Get(i)->Clone());
        result_non_interpolable_values[i] = end_non_interpolable_list.Get(i);
      }
    }
  }

  return PairwiseInterpolationValue(
      std::move(result_start_interpolable_list),
      std::move(result_end_interpolable_list),
      NonInterpolableList::Create(std::move(result_non_interpolable_values)));
}

static void RepeatToLength(InterpolationValue& value, size_t length) {
  InterpolableList& interpolable_list =
      ToInterpolableList(*value.interpolable_value);
  NonInterpolableList& non_interpolable_list =
      ToNonInterpolableList(*value.non_interpolable_value);
  size_t current_length = interpolable_list.length();
  DCHECK_GT(current_length, 0U);
  if (current_length == length)
    return;
  DCHECK_LT(current_length, length);
  std::unique_ptr<InterpolableList> new_interpolable_list =
      InterpolableList::Create(length);
  Vector<scoped_refptr<NonInterpolableValue>> new_non_interpolable_values(
      length);
  for (size_t i = length; i-- > 0;) {
    new_interpolable_list->Set(
        i, i < current_length
               ? std::move(interpolable_list.GetMutable(i))
               : interpolable_list.Get(i % current_length)->Clone());
    new_non_interpolable_values[i] =
        non_interpolable_list.Get(i % current_length);
  }
  value.interpolable_value = std::move(new_interpolable_list);
  value.non_interpolable_value =
      NonInterpolableList::Create(std::move(new_non_interpolable_values));
}

// This helper function makes value the same length as length_value by
// CloneAndZero-ing the additional items from length_value into value.
static void PadToSameLength(InterpolationValue& value,
                            const InterpolationValue& length_value) {
  InterpolableList& interpolable_list =
      ToInterpolableList(*value.interpolable_value);
  NonInterpolableList& non_interpolable_list =
      ToNonInterpolableList(*value.non_interpolable_value);
  const size_t current_length = interpolable_list.length();
  InterpolableList& target_interpolable_list =
      ToInterpolableList(*length_value.interpolable_value);
  NonInterpolableList& target_non_interpolable_list =
      ToNonInterpolableList(*length_value.non_interpolable_value);
  const size_t target_length = target_interpolable_list.length();
  DCHECK_LT(current_length, target_length);
  std::unique_ptr<InterpolableList> new_interpolable_list =
      InterpolableList::Create(target_length);
  Vector<scoped_refptr<NonInterpolableValue>> new_non_interpolable_values(
      target_length);
  size_t index = 0;
  for (; index < current_length; index++) {
    new_interpolable_list->Set(index,
                               std::move(interpolable_list.GetMutable(index)));
    new_non_interpolable_values[index] = non_interpolable_list.Get(index);
  }
  for (; index < target_length; index++) {
    new_interpolable_list->Set(
        index, target_interpolable_list.Get(index)->CloneAndZero());
    new_non_interpolable_values[index] =
        target_non_interpolable_list.Get(index);
  }
  value.interpolable_value = std::move(new_interpolable_list);
  value.non_interpolable_value =
      NonInterpolableList::Create(std::move(new_non_interpolable_values));
}

static bool NonInterpolableListsAreCompatible(
    const NonInterpolableList& a,
    const NonInterpolableList& b,
    size_t length,
    ListInterpolationFunctions::LengthMatchingStrategy length_matching_strategy,
    ListInterpolationFunctions::NonInterpolableValuesAreCompatibleCallback
        non_interpolable_values_are_compatible) {
  for (size_t i = 0; i < length; i++) {
    if (length_matching_strategy ==
            ListInterpolationFunctions::LengthMatchingStrategy::
                kLowestCommonMultiple ||
        (i < a.length() && i < b.length())) {
      if (!non_interpolable_values_are_compatible(a.Get(i % a.length()),
                                                  b.Get(i % b.length()))) {
        return false;
      }
    }
  }
  return true;
}

void ListInterpolationFunctions::Composite(
    UnderlyingValueOwner& underlying_value_owner,
    double underlying_fraction,
    const InterpolationType& type,
    const InterpolationValue& value,
    LengthMatchingStrategy length_matching_strategy,
    NonInterpolableValuesAreCompatibleCallback
        non_interpolable_values_are_compatible,
    CompositeItemCallback composite_item) {
  const size_t underlying_length =
      ToInterpolableList(*underlying_value_owner.Value().interpolable_value)
          .length();
  if (underlying_length == 0) {
    DCHECK(!underlying_value_owner.Value().non_interpolable_value);
    underlying_value_owner.Set(type, value);
    return;
  }

  const InterpolableList& interpolable_list =
      ToInterpolableList(*value.interpolable_value);
  const size_t value_length = interpolable_list.length();
  if (value_length == 0) {
    DCHECK(!value.non_interpolable_value);
    underlying_value_owner.MutableValue().interpolable_value->Scale(
        underlying_fraction);
    return;
  }

  const NonInterpolableList& non_interpolable_list =
      ToNonInterpolableList(*value.non_interpolable_value);
  const size_t final_length =
      MatchLengths(underlying_length, value_length, length_matching_strategy);
  if (!NonInterpolableListsAreCompatible(
          ToNonInterpolableList(
              *underlying_value_owner.Value().non_interpolable_value),
          non_interpolable_list, final_length, length_matching_strategy,
          non_interpolable_values_are_compatible)) {
    underlying_value_owner.Set(type, value);
    return;
  }

  InterpolationValue& underlying_value = underlying_value_owner.MutableValue();
  if (length_matching_strategy ==
      LengthMatchingStrategy::kLowestCommonMultiple) {
    if (underlying_length < final_length) {
      RepeatToLength(underlying_value, final_length);
    }
    InterpolableList& underlying_interpolable_list =
        ToInterpolableList(*underlying_value.interpolable_value);
    NonInterpolableList& underlying_non_interpolable_list =
        ToNonInterpolableList(*underlying_value.non_interpolable_value);

    for (size_t i = 0; i < final_length; i++) {
      composite_item(underlying_interpolable_list.GetMutable(i),
                     underlying_non_interpolable_list.GetMutable(i),
                     underlying_fraction,
                     *interpolable_list.Get(i % value_length),
                     non_interpolable_list.Get(i % value_length));
    }
  } else {
    DCHECK_EQ(length_matching_strategy, LengthMatchingStrategy::kPadToLargest);
    if (underlying_length < final_length) {
      DCHECK_EQ(value_length, final_length);
      PadToSameLength(underlying_value, value);
    }
    InterpolableList& underlying_interpolable_list =
        ToInterpolableList(*underlying_value.interpolable_value);
    NonInterpolableList& underlying_non_interpolable_list =
        ToNonInterpolableList(*underlying_value.non_interpolable_value);

    for (size_t i = 0; i < value_length; i++) {
      composite_item(underlying_interpolable_list.GetMutable(i),
                     underlying_non_interpolable_list.GetMutable(i),
                     underlying_fraction, *interpolable_list.Get(i),
                     non_interpolable_list.Get(i));
    }
    for (size_t i = value_length; i < final_length; i++) {
      underlying_interpolable_list.GetMutable(i)->Scale(underlying_fraction);
    }
  }
}

}  // namespace blink

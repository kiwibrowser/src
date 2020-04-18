// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css_filter_list_interpolation_type.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/animation/filter_interpolation_functions.h"
#include "third_party/blink/renderer/core/animation/list_interpolation_functions.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_state.h"
#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

namespace {

const FilterOperations& GetFilterList(const CSSProperty& property,
                                      const ComputedStyle& style) {
  switch (property.PropertyID()) {
    default:
      NOTREACHED();
      FALLTHROUGH;
    case CSSPropertyBackdropFilter:
      return style.BackdropFilter();
    case CSSPropertyFilter:
      return style.Filter();
  }
}

void SetFilterList(const CSSProperty& property,
                   ComputedStyle& style,
                   const FilterOperations& filter_operations) {
  switch (property.PropertyID()) {
    case CSSPropertyBackdropFilter:
      style.SetBackdropFilter(filter_operations);
      break;
    case CSSPropertyFilter:
      style.SetFilter(filter_operations);
      break;
    default:
      NOTREACHED();
      break;
  }
}

class UnderlyingFilterListChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  static std::unique_ptr<UnderlyingFilterListChecker> Create(
      scoped_refptr<NonInterpolableList> non_interpolable_list) {
    return base::WrapUnique(
        new UnderlyingFilterListChecker(std::move(non_interpolable_list)));
  }

  bool IsValid(const StyleResolverState&,
               const InterpolationValue& underlying) const final {
    const NonInterpolableList& underlying_non_interpolable_list =
        ToNonInterpolableList(*underlying.non_interpolable_value);
    if (non_interpolable_list_->length() !=
        underlying_non_interpolable_list.length())
      return false;
    for (size_t i = 0; i < non_interpolable_list_->length(); i++) {
      if (!FilterInterpolationFunctions::FiltersAreCompatible(
              *non_interpolable_list_->Get(i),
              *underlying_non_interpolable_list.Get(i)))
        return false;
    }
    return true;
  }

 private:
  UnderlyingFilterListChecker(
      scoped_refptr<NonInterpolableList> non_interpolable_list)
      : non_interpolable_list_(std::move(non_interpolable_list)) {}

  scoped_refptr<NonInterpolableList> non_interpolable_list_;
};

class InheritedFilterListChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  static std::unique_ptr<InheritedFilterListChecker> Create(
      const CSSProperty& property,
      const FilterOperations& filter_operations) {
    return base::WrapUnique(
        new InheritedFilterListChecker(property, filter_operations));
  }

  bool IsValid(const StyleResolverState& state,
               const InterpolationValue&) const final {
    const FilterOperations& filter_operations =
        filter_operations_wrapper_->Operations();
    return filter_operations == GetFilterList(property_, *state.ParentStyle());
  }

 private:
  InheritedFilterListChecker(const CSSProperty& property,
                             const FilterOperations& filter_operations)
      : property_(property),
        filter_operations_wrapper_(
            FilterOperationsWrapper::Create(filter_operations)) {}

  const CSSProperty& property_;
  Persistent<FilterOperationsWrapper> filter_operations_wrapper_;
};

InterpolationValue ConvertFilterList(const FilterOperations& filter_operations,
                                     double zoom) {
  size_t length = filter_operations.size();
  std::unique_ptr<InterpolableList> interpolable_list =
      InterpolableList::Create(length);
  Vector<scoped_refptr<NonInterpolableValue>> non_interpolable_values(length);
  for (size_t i = 0; i < length; i++) {
    InterpolationValue filter_result =
        FilterInterpolationFunctions::MaybeConvertFilter(
            *filter_operations.Operations()[i], zoom);
    if (!filter_result)
      return nullptr;
    interpolable_list->Set(i, std::move(filter_result.interpolable_value));
    non_interpolable_values[i] =
        std::move(filter_result.non_interpolable_value);
  }
  return InterpolationValue(
      std::move(interpolable_list),
      NonInterpolableList::Create(std::move(non_interpolable_values)));
}

}  // namespace

InterpolationValue CSSFilterListInterpolationType::MaybeConvertNeutral(
    const InterpolationValue& underlying,
    ConversionCheckers& conversion_checkers) const {
  // const_cast for taking refs.
  NonInterpolableList& non_interpolable_list = const_cast<NonInterpolableList&>(
      ToNonInterpolableList(*underlying.non_interpolable_value));
  conversion_checkers.push_back(
      UnderlyingFilterListChecker::Create(&non_interpolable_list));
  return InterpolationValue(underlying.interpolable_value->CloneAndZero(),
                            &non_interpolable_list);
}

InterpolationValue CSSFilterListInterpolationType::MaybeConvertInitial(
    const StyleResolverState&,
    ConversionCheckers& conversion_checkers) const {
  return ConvertFilterList(
      GetFilterList(CssProperty(), ComputedStyle::InitialStyle()), 1);
}

InterpolationValue CSSFilterListInterpolationType::MaybeConvertInherit(
    const StyleResolverState& state,
    ConversionCheckers& conversion_checkers) const {
  const FilterOperations& inherited_filter_operations =
      GetFilterList(CssProperty(), *state.ParentStyle());
  conversion_checkers.push_back(InheritedFilterListChecker::Create(
      CssProperty(), inherited_filter_operations));
  return ConvertFilterList(inherited_filter_operations,
                           state.Style()->EffectiveZoom());
}

InterpolationValue CSSFilterListInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState*,
    ConversionCheckers&) const {
  if (value.IsIdentifierValue() &&
      ToCSSIdentifierValue(value).GetValueID() == CSSValueNone)
    return InterpolationValue(InterpolableList::Create(0),
                              NonInterpolableList::Create());

  if (!value.IsBaseValueList())
    return nullptr;

  const CSSValueList& list = ToCSSValueList(value);
  size_t length = list.length();
  std::unique_ptr<InterpolableList> interpolable_list =
      InterpolableList::Create(length);
  Vector<scoped_refptr<NonInterpolableValue>> non_interpolable_values(length);
  for (size_t i = 0; i < length; i++) {
    InterpolationValue item_result =
        FilterInterpolationFunctions::MaybeConvertCSSFilter(list.Item(i));
    if (!item_result)
      return nullptr;
    interpolable_list->Set(i, std::move(item_result.interpolable_value));
    non_interpolable_values[i] = std::move(item_result.non_interpolable_value);
  }
  return InterpolationValue(
      std::move(interpolable_list),
      NonInterpolableList::Create(std::move(non_interpolable_values)));
}

InterpolationValue
CSSFilterListInterpolationType::MaybeConvertStandardPropertyUnderlyingValue(
    const ComputedStyle& style) const {
  return ConvertFilterList(GetFilterList(CssProperty(), style),
                           style.EffectiveZoom());
}

PairwiseInterpolationValue CSSFilterListInterpolationType::MaybeMergeSingles(
    InterpolationValue&& start,
    InterpolationValue&& end) const {
  NonInterpolableList& start_non_interpolable_list =
      ToNonInterpolableList(*start.non_interpolable_value);
  NonInterpolableList& end_non_interpolable_list =
      ToNonInterpolableList(*end.non_interpolable_value);
  size_t start_length = start_non_interpolable_list.length();
  size_t end_length = end_non_interpolable_list.length();

  for (size_t i = 0; i < start_length && i < end_length; i++) {
    if (!FilterInterpolationFunctions::FiltersAreCompatible(
            *start_non_interpolable_list.Get(i),
            *end_non_interpolable_list.Get(i)))
      return nullptr;
  }

  if (start_length == end_length) {
    return PairwiseInterpolationValue(std::move(start.interpolable_value),
                                      std::move(end.interpolable_value),
                                      std::move(start.non_interpolable_value));
  }

  // Extend the shorter InterpolableList with neutral values that are compatible
  // with corresponding filters in the longer list.
  InterpolationValue& shorter = start_length < end_length ? start : end;
  InterpolationValue& longer = start_length < end_length ? end : start;
  size_t shorter_length =
      ToNonInterpolableList(*shorter.non_interpolable_value).length();
  size_t longer_length =
      ToNonInterpolableList(*longer.non_interpolable_value).length();
  InterpolableList& shorter_interpolable_list =
      ToInterpolableList(*shorter.interpolable_value);
  const NonInterpolableList& longer_non_interpolable_list =
      ToNonInterpolableList(*longer.non_interpolable_value);
  std::unique_ptr<InterpolableList> extended_interpolable_list =
      InterpolableList::Create(longer_length);
  for (size_t i = 0; i < longer_length; i++) {
    if (i < shorter_length)
      extended_interpolable_list->Set(
          i, std::move(shorter_interpolable_list.GetMutable(i)));
    else
      extended_interpolable_list->Set(
          i, FilterInterpolationFunctions::CreateNoneValue(
                 *longer_non_interpolable_list.Get(i)));
  }
  shorter.interpolable_value = std::move(extended_interpolable_list);

  return PairwiseInterpolationValue(std::move(start.interpolable_value),
                                    std::move(end.interpolable_value),
                                    std::move(longer.non_interpolable_value));
}

void CSSFilterListInterpolationType::Composite(
    UnderlyingValueOwner& underlying_value_owner,
    double underlying_fraction,
    const InterpolationValue& value,
    double interpolation_fraction) const {
  const NonInterpolableList& underlying_non_interpolable_list =
      ToNonInterpolableList(
          *underlying_value_owner.Value().non_interpolable_value);
  const NonInterpolableList& non_interpolable_list =
      ToNonInterpolableList(*value.non_interpolable_value);
  size_t underlying_length = underlying_non_interpolable_list.length();
  size_t length = non_interpolable_list.length();

  for (size_t i = 0; i < underlying_length && i < length; i++) {
    if (!FilterInterpolationFunctions::FiltersAreCompatible(
            *underlying_non_interpolable_list.Get(i),
            *non_interpolable_list.Get(i))) {
      underlying_value_owner.Set(*this, value);
      return;
    }
  }

  InterpolableList& underlying_interpolable_list = ToInterpolableList(
      *underlying_value_owner.MutableValue().interpolable_value);
  const InterpolableList& interpolable_list =
      ToInterpolableList(*value.interpolable_value);
  DCHECK_EQ(underlying_length, underlying_interpolable_list.length());
  DCHECK_EQ(length, interpolable_list.length());

  for (size_t i = 0; i < length && i < underlying_length; i++)
    underlying_interpolable_list.GetMutable(i)->ScaleAndAdd(
        underlying_fraction, *interpolable_list.Get(i));

  if (length <= underlying_length)
    return;

  std::unique_ptr<InterpolableList> extended_interpolable_list =
      InterpolableList::Create(length);
  for (size_t i = 0; i < length; i++) {
    if (i < underlying_length)
      extended_interpolable_list->Set(
          i, std::move(underlying_interpolable_list.GetMutable(i)));
    else
      extended_interpolable_list->Set(i, interpolable_list.Get(i)->Clone());
  }
  underlying_value_owner.MutableValue().interpolable_value =
      std::move(extended_interpolable_list);
  // const_cast to take a ref.
  underlying_value_owner.MutableValue().non_interpolable_value =
      const_cast<NonInterpolableValue*>(value.non_interpolable_value.get());
}

void CSSFilterListInterpolationType::ApplyStandardPropertyValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue* non_interpolable_value,
    StyleResolverState& state) const {
  const InterpolableList& interpolable_list =
      ToInterpolableList(interpolable_value);
  const NonInterpolableList& non_interpolable_list =
      ToNonInterpolableList(*non_interpolable_value);
  size_t length = interpolable_list.length();
  DCHECK_EQ(length, non_interpolable_list.length());

  FilterOperations filter_operations;
  filter_operations.Operations().ReserveCapacity(length);
  for (size_t i = 0; i < length; i++) {
    filter_operations.Operations().push_back(
        FilterInterpolationFunctions::CreateFilter(
            *interpolable_list.Get(i), *non_interpolable_list.Get(i), state));
  }
  SetFilterList(CssProperty(), *state.Style(), std::move(filter_operations));
}

}  // namespace blink

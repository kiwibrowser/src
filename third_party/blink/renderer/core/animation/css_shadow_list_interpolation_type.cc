// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css_shadow_list_interpolation_type.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/animation/list_interpolation_functions.h"
#include "third_party/blink/renderer/core/animation/shadow_interpolation_functions.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/resolver/style_builder.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_state.h"
#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/style/shadow_list.h"

namespace blink {

namespace {
const ShadowList* GetShadowList(const CSSProperty& property,
                                const ComputedStyle& style) {
  switch (property.PropertyID()) {
    case CSSPropertyBoxShadow:
      return style.BoxShadow();
    case CSSPropertyTextShadow:
      return style.TextShadow();
    default:
      NOTREACHED();
      return nullptr;
  }
}
}  // namespace

InterpolationValue CSSShadowListInterpolationType::ConvertShadowList(
    const ShadowList* shadow_list,
    double zoom) const {
  if (!shadow_list)
    return CreateNeutralValue();
  const ShadowDataVector& shadows = shadow_list->Shadows();
  return ListInterpolationFunctions::CreateList(
      shadows.size(), [&shadows, zoom](size_t index) {
        return ShadowInterpolationFunctions::ConvertShadowData(shadows[index],
                                                               zoom);
      });
}

InterpolationValue CSSShadowListInterpolationType::CreateNeutralValue() const {
  return ListInterpolationFunctions::CreateEmptyList();
}

InterpolationValue CSSShadowListInterpolationType::MaybeConvertNeutral(
    const InterpolationValue&,
    ConversionCheckers&) const {
  return CreateNeutralValue();
}

InterpolationValue CSSShadowListInterpolationType::MaybeConvertInitial(
    const StyleResolverState&,
    ConversionCheckers&) const {
  return CreateNeutralValue();
}

class InheritedShadowListChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  static std::unique_ptr<InheritedShadowListChecker> Create(
      const CSSProperty& property,
      scoped_refptr<ShadowList> shadow_list) {
    return base::WrapUnique(
        new InheritedShadowListChecker(property, std::move(shadow_list)));
  }

 private:
  InheritedShadowListChecker(const CSSProperty& property,
                             scoped_refptr<ShadowList> shadow_list)
      : property_(property), shadow_list_(std::move(shadow_list)) {}

  bool IsValid(const StyleResolverState& state,
               const InterpolationValue& underlying) const final {
    const ShadowList* inherited_shadow_list =
        GetShadowList(property_, *state.ParentStyle());
    if (!inherited_shadow_list && !shadow_list_)
      return true;
    if (!inherited_shadow_list || !shadow_list_)
      return false;
    return *inherited_shadow_list == *shadow_list_;
  }

  const CSSProperty& property_;
  scoped_refptr<ShadowList> shadow_list_;
};

InterpolationValue CSSShadowListInterpolationType::MaybeConvertInherit(
    const StyleResolverState& state,
    ConversionCheckers& conversion_checkers) const {
  if (!state.ParentStyle())
    return nullptr;
  const ShadowList* inherited_shadow_list =
      GetShadowList(CssProperty(), *state.ParentStyle());
  conversion_checkers.push_back(InheritedShadowListChecker::Create(
      CssProperty(),
      const_cast<ShadowList*>(inherited_shadow_list)));  // Take ref.
  return ConvertShadowList(inherited_shadow_list,
                           state.ParentStyle()->EffectiveZoom());
}

InterpolationValue CSSShadowListInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState*,
    ConversionCheckers&) const {
  if (value.IsIdentifierValue() &&
      ToCSSIdentifierValue(value).GetValueID() == CSSValueNone)
    return CreateNeutralValue();

  if (!value.IsBaseValueList())
    return nullptr;

  const CSSValueList& value_list = ToCSSValueList(value);
  return ListInterpolationFunctions::CreateList(
      value_list.length(), [&value_list](size_t index) {
        return ShadowInterpolationFunctions::MaybeConvertCSSValue(
            value_list.Item(index));
      });
}

PairwiseInterpolationValue CSSShadowListInterpolationType::MaybeMergeSingles(
    InterpolationValue&& start,
    InterpolationValue&& end) const {
  return ListInterpolationFunctions::MaybeMergeSingles(
      std::move(start), std::move(end),
      ListInterpolationFunctions::LengthMatchingStrategy::kPadToLargest,
      ShadowInterpolationFunctions::MaybeMergeSingles);
}

InterpolationValue
CSSShadowListInterpolationType::MaybeConvertStandardPropertyUnderlyingValue(
    const ComputedStyle& style) const {
  return ConvertShadowList(GetShadowList(CssProperty(), style),
                           style.EffectiveZoom());
}

void CSSShadowListInterpolationType::Composite(
    UnderlyingValueOwner& underlying_value_owner,
    double underlying_fraction,
    const InterpolationValue& value,
    double interpolation_fraction) const {
  ListInterpolationFunctions::Composite(
      underlying_value_owner, underlying_fraction, *this, value,
      ListInterpolationFunctions::LengthMatchingStrategy::kPadToLargest,
      ShadowInterpolationFunctions::NonInterpolableValuesAreCompatible,
      ShadowInterpolationFunctions::Composite);
}

static scoped_refptr<ShadowList> CreateShadowList(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue* non_interpolable_value,
    const StyleResolverState& state) {
  const InterpolableList& interpolable_list =
      ToInterpolableList(interpolable_value);
  size_t length = interpolable_list.length();
  if (length == 0)
    return nullptr;
  const NonInterpolableList& non_interpolable_list =
      ToNonInterpolableList(*non_interpolable_value);
  ShadowDataVector shadows;
  for (size_t i = 0; i < length; i++)
    shadows.push_back(ShadowInterpolationFunctions::CreateShadowData(
        *interpolable_list.Get(i), non_interpolable_list.Get(i), state));
  return ShadowList::Adopt(shadows);
}

void CSSShadowListInterpolationType::ApplyStandardPropertyValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue* non_interpolable_value,
    StyleResolverState& state) const {
  scoped_refptr<ShadowList> shadow_list =
      CreateShadowList(interpolable_value, non_interpolable_value, state);
  switch (CssProperty().PropertyID()) {
    case CSSPropertyBoxShadow:
      state.Style()->SetBoxShadow(std::move(shadow_list));
      return;
    case CSSPropertyTextShadow:
      state.Style()->SetTextShadow(std::move(shadow_list));
      return;
    default:
      NOTREACHED();
  }
}

}  // namespace blink

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css_font_weight_interpolation_type.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/css/css_primitive_value_mappings.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_state.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"

namespace blink {

class InheritedFontWeightChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  static std::unique_ptr<InheritedFontWeightChecker> Create(
      FontSelectionValue font_weight) {
    return base::WrapUnique(new InheritedFontWeightChecker(font_weight));
  }

 private:
  InheritedFontWeightChecker(FontSelectionValue font_weight)
      : font_weight_(font_weight) {}

  bool IsValid(const StyleResolverState& state,
               const InterpolationValue&) const final {
    return font_weight_ == state.ParentStyle()->GetFontWeight();
  }

  const double font_weight_;
};

InterpolationValue CSSFontWeightInterpolationType::CreateFontWeightValue(
    FontSelectionValue font_weight) const {
  return InterpolationValue(InterpolableNumber::Create(font_weight));
}

InterpolationValue CSSFontWeightInterpolationType::MaybeConvertNeutral(
    const InterpolationValue&,
    ConversionCheckers&) const {
  return InterpolationValue(InterpolableNumber::Create(0));
}

InterpolationValue CSSFontWeightInterpolationType::MaybeConvertInitial(
    const StyleResolverState&,
    ConversionCheckers& conversion_checkers) const {
  return CreateFontWeightValue(NormalWeightValue());
}

InterpolationValue CSSFontWeightInterpolationType::MaybeConvertInherit(
    const StyleResolverState& state,
    ConversionCheckers& conversion_checkers) const {
  if (!state.ParentStyle())
    return nullptr;
  FontSelectionValue inherited_font_weight =
      state.ParentStyle()->GetFontWeight();
  conversion_checkers.push_back(
      InheritedFontWeightChecker::Create(inherited_font_weight));
  return CreateFontWeightValue(inherited_font_weight);
}

InterpolationValue CSSFontWeightInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState* state,
    ConversionCheckers& conversion_checkers) const {
  if (value.IsPrimitiveValue()) {
    return CreateFontWeightValue(
        FontSelectionValue(ToCSSPrimitiveValue(value).GetFloatValue()));
  }

  CHECK(value.IsIdentifierValue());
  const CSSIdentifierValue& identifier_value = ToCSSIdentifierValue(value);
  CSSValueID keyword = identifier_value.GetValueID();

  switch (keyword) {
    case CSSValueInvalid:
      return nullptr;
    case CSSValueNormal:
      return CreateFontWeightValue(NormalWeightValue());
    case CSSValueBold:
      return CreateFontWeightValue(BoldWeightValue());

    case CSSValueBolder:
    case CSSValueLighter: {
      DCHECK(state);
      FontSelectionValue inherited_font_weight =
          state->ParentStyle()->GetFontWeight();
      conversion_checkers.push_back(
          InheritedFontWeightChecker::Create(inherited_font_weight));
      if (keyword == CSSValueBolder) {
        return CreateFontWeightValue(
            FontDescription::BolderWeight(inherited_font_weight));
      }
      return CreateFontWeightValue(
          FontDescription::LighterWeight(inherited_font_weight));
    }
    default:
      NOTREACHED();
      return nullptr;
  }
}

InterpolationValue
CSSFontWeightInterpolationType::MaybeConvertStandardPropertyUnderlyingValue(
    const ComputedStyle& style) const {
  return CreateFontWeightValue(style.GetFontWeight());
}

void CSSFontWeightInterpolationType::ApplyStandardPropertyValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue*,
    StyleResolverState& state) const {
  state.GetFontBuilder().SetWeight(FontSelectionValue(
      clampTo(ToInterpolableNumber(interpolable_value).Value(),
              MinWeightValue(), MaxWeightValue())));
}

}  // namespace blink

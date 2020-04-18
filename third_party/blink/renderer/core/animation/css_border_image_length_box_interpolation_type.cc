// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css_border_image_length_box_interpolation_type.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/animation/length_interpolation_functions.h"
#include "third_party/blink/renderer/core/animation/side_index.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_quad_value.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_state.h"
#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

namespace {

enum class SideType {
  kNumber,
  kAuto,
  kLength,
};

SideType GetSideType(const BorderImageLength& side) {
  if (side.IsNumber()) {
    return SideType::kNumber;
  }
  if (side.length().IsAuto()) {
    return SideType::kAuto;
  }
  DCHECK(side.length().IsSpecified());
  return SideType::kLength;
}

SideType GetSideType(const CSSValue& side) {
  if (side.IsPrimitiveValue() && ToCSSPrimitiveValue(side).IsNumber()) {
    return SideType::kNumber;
  }
  if (side.IsIdentifierValue() &&
      ToCSSIdentifierValue(side).GetValueID() == CSSValueAuto) {
    return SideType::kAuto;
  }
  return SideType::kLength;
}

struct SideTypes {
  explicit SideTypes(const BorderImageLengthBox& box) {
    type[kSideTop] = GetSideType(box.Top());
    type[kSideRight] = GetSideType(box.Right());
    type[kSideBottom] = GetSideType(box.Bottom());
    type[kSideLeft] = GetSideType(box.Left());
  }
  explicit SideTypes(const CSSQuadValue& quad) {
    type[kSideTop] = GetSideType(*quad.Top());
    type[kSideRight] = GetSideType(*quad.Right());
    type[kSideBottom] = GetSideType(*quad.Bottom());
    type[kSideLeft] = GetSideType(*quad.Left());
  }

  bool operator==(const SideTypes& other) const {
    for (size_t i = 0; i < kSideIndexCount; i++) {
      if (type[i] != other.type[i])
        return false;
    }
    return true;
  }
  bool operator!=(const SideTypes& other) const { return !(*this == other); }

  SideType type[kSideIndexCount];
};

const BorderImageLengthBox& GetBorderImageLengthBox(
    const CSSProperty& property,
    const ComputedStyle& style) {
  switch (property.PropertyID()) {
    case CSSPropertyBorderImageOutset:
      return style.BorderImageOutset();
    case CSSPropertyBorderImageWidth:
      return style.BorderImageWidth();
    case CSSPropertyWebkitMaskBoxImageOutset:
      return style.MaskBoxImageOutset();
    case CSSPropertyWebkitMaskBoxImageWidth:
      return style.MaskBoxImageWidth();
    default:
      NOTREACHED();
      return GetBorderImageLengthBox(
          CSSProperty::Get(CSSPropertyBorderImageOutset),
          ComputedStyle::InitialStyle());
  }
}

void SetBorderImageLengthBox(const CSSProperty& property,
                             ComputedStyle& style,
                             const BorderImageLengthBox& box) {
  switch (property.PropertyID()) {
    case CSSPropertyBorderImageOutset:
      style.SetBorderImageOutset(box);
      break;
    case CSSPropertyWebkitMaskBoxImageOutset:
      style.SetMaskBoxImageOutset(box);
      break;
    case CSSPropertyBorderImageWidth:
      style.SetBorderImageWidth(box);
      break;
    case CSSPropertyWebkitMaskBoxImageWidth:
      style.SetMaskBoxImageWidth(box);
      break;
    default:
      NOTREACHED();
      break;
  }
}

}  // namespace

class CSSBorderImageLengthBoxNonInterpolableValue
    : public NonInterpolableValue {
 public:
  static scoped_refptr<CSSBorderImageLengthBoxNonInterpolableValue> Create(
      const SideTypes& side_types,
      Vector<scoped_refptr<NonInterpolableValue>>&&
          side_non_interpolable_values) {
    return base::AdoptRef(new CSSBorderImageLengthBoxNonInterpolableValue(
        side_types, std::move(side_non_interpolable_values)));
  }

  scoped_refptr<CSSBorderImageLengthBoxNonInterpolableValue> Clone() {
    return base::AdoptRef(new CSSBorderImageLengthBoxNonInterpolableValue(
        side_types_, Vector<scoped_refptr<NonInterpolableValue>>(
                         side_non_interpolable_values_)));
  }

  const SideTypes& GetSideTypes() const { return side_types_; }
  const Vector<scoped_refptr<NonInterpolableValue>>& SideNonInterpolableValues()
      const {
    return side_non_interpolable_values_;
  }
  Vector<scoped_refptr<NonInterpolableValue>>& SideNonInterpolableValues() {
    return side_non_interpolable_values_;
  }

  DECLARE_NON_INTERPOLABLE_VALUE_TYPE();

 private:
  CSSBorderImageLengthBoxNonInterpolableValue(
      const SideTypes& side_types,
      Vector<scoped_refptr<NonInterpolableValue>>&&
          side_non_interpolable_values)
      : side_types_(side_types),
        side_non_interpolable_values_(side_non_interpolable_values) {
    DCHECK_EQ(side_non_interpolable_values_.size(), kSideIndexCount);
  }

  const SideTypes side_types_;
  Vector<scoped_refptr<NonInterpolableValue>> side_non_interpolable_values_;
};

DEFINE_NON_INTERPOLABLE_VALUE_TYPE(CSSBorderImageLengthBoxNonInterpolableValue);
DEFINE_NON_INTERPOLABLE_VALUE_TYPE_CASTS(
    CSSBorderImageLengthBoxNonInterpolableValue);

namespace {

class UnderlyingSideTypesChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  static std::unique_ptr<UnderlyingSideTypesChecker> Create(
      const SideTypes& underlying_side_types) {
    return base::WrapUnique(
        new UnderlyingSideTypesChecker(underlying_side_types));
  }

  static SideTypes GetUnderlyingSideTypes(
      const InterpolationValue& underlying) {
    return ToCSSBorderImageLengthBoxNonInterpolableValue(
               *underlying.non_interpolable_value)
        .GetSideTypes();
  }

 private:
  UnderlyingSideTypesChecker(const SideTypes& underlying_side_types)
      : underlying_side_types_(underlying_side_types) {}

  bool IsValid(const StyleResolverState&,
               const InterpolationValue& underlying) const final {
    return underlying_side_types_ == GetUnderlyingSideTypes(underlying);
  }

  const SideTypes underlying_side_types_;
};

class InheritedSideTypesChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  static std::unique_ptr<InheritedSideTypesChecker> Create(
      const CSSProperty& property,
      const SideTypes& inherited_side_types) {
    return base::WrapUnique(
        new InheritedSideTypesChecker(property, inherited_side_types));
  }

 private:
  InheritedSideTypesChecker(const CSSProperty& property,
                            const SideTypes& inherited_side_types)
      : property_(property), inherited_side_types_(inherited_side_types) {}

  bool IsValid(const StyleResolverState& state,
               const InterpolationValue& underlying) const final {
    return inherited_side_types_ ==
           SideTypes(GetBorderImageLengthBox(property_, *state.ParentStyle()));
  }

  const CSSProperty& property_;
  const SideTypes inherited_side_types_;
};

InterpolationValue ConvertBorderImageLengthBox(const BorderImageLengthBox& box,
                                               double zoom) {
  std::unique_ptr<InterpolableList> list =
      InterpolableList::Create(kSideIndexCount);
  Vector<scoped_refptr<NonInterpolableValue>> non_interpolable_values(
      kSideIndexCount);
  const BorderImageLength* sides[kSideIndexCount] = {};
  sides[kSideTop] = &box.Top();
  sides[kSideRight] = &box.Right();
  sides[kSideBottom] = &box.Bottom();
  sides[kSideLeft] = &box.Left();

  for (size_t i = 0; i < kSideIndexCount; i++) {
    const BorderImageLength& side = *sides[i];
    if (side.IsNumber()) {
      list->Set(i, InterpolableNumber::Create(side.Number()));
    } else if (side.length().IsAuto()) {
      list->Set(i, InterpolableList::Create(0));
    } else {
      InterpolationValue converted_side =
          LengthInterpolationFunctions::MaybeConvertLength(side.length(), zoom);
      if (!converted_side)
        return nullptr;
      list->Set(i, std::move(converted_side.interpolable_value));
      non_interpolable_values[i] =
          std::move(converted_side.non_interpolable_value);
    }
  }

  return InterpolationValue(
      std::move(list), CSSBorderImageLengthBoxNonInterpolableValue::Create(
                           SideTypes(box), std::move(non_interpolable_values)));
}

}  // namespace

InterpolationValue
CSSBorderImageLengthBoxInterpolationType::MaybeConvertNeutral(
    const InterpolationValue& underlying,
    ConversionCheckers& conversion_checkers) const {
  SideTypes underlying_side_types =
      UnderlyingSideTypesChecker::GetUnderlyingSideTypes(underlying);
  conversion_checkers.push_back(
      UnderlyingSideTypesChecker::Create(underlying_side_types));
  return InterpolationValue(underlying.interpolable_value->CloneAndZero(),
                            ToCSSBorderImageLengthBoxNonInterpolableValue(
                                *underlying.non_interpolable_value)
                                .Clone());
}

InterpolationValue
CSSBorderImageLengthBoxInterpolationType::MaybeConvertInitial(
    const StyleResolverState&,
    ConversionCheckers&) const {
  return ConvertBorderImageLengthBox(
      GetBorderImageLengthBox(CssProperty(), ComputedStyle::InitialStyle()), 1);
}

InterpolationValue
CSSBorderImageLengthBoxInterpolationType::MaybeConvertInherit(
    const StyleResolverState& state,
    ConversionCheckers& conversion_checkers) const {
  const BorderImageLengthBox& inherited =
      GetBorderImageLengthBox(CssProperty(), *state.ParentStyle());
  conversion_checkers.push_back(
      InheritedSideTypesChecker::Create(CssProperty(), SideTypes(inherited)));
  return ConvertBorderImageLengthBox(inherited,
                                     state.ParentStyle()->EffectiveZoom());
}

InterpolationValue CSSBorderImageLengthBoxInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState*,
    ConversionCheckers&) const {
  if (!value.IsQuadValue())
    return nullptr;

  const CSSQuadValue& quad = ToCSSQuadValue(value);
  std::unique_ptr<InterpolableList> list =
      InterpolableList::Create(kSideIndexCount);
  Vector<scoped_refptr<NonInterpolableValue>> non_interpolable_values(
      kSideIndexCount);
  const CSSValue* sides[kSideIndexCount] = {};
  sides[kSideTop] = quad.Top();
  sides[kSideRight] = quad.Right();
  sides[kSideBottom] = quad.Bottom();
  sides[kSideLeft] = quad.Left();

  for (size_t i = 0; i < kSideIndexCount; i++) {
    const CSSValue& side = *sides[i];
    if (side.IsPrimitiveValue() && ToCSSPrimitiveValue(side).IsNumber()) {
      list->Set(i, InterpolableNumber::Create(
                       ToCSSPrimitiveValue(side).GetDoubleValue()));
    } else if (side.IsIdentifierValue() &&
               ToCSSIdentifierValue(side).GetValueID() == CSSValueAuto) {
      list->Set(i, InterpolableList::Create(0));
    } else {
      InterpolationValue converted_side =
          LengthInterpolationFunctions::MaybeConvertCSSValue(side);
      if (!converted_side)
        return nullptr;
      list->Set(i, std::move(converted_side.interpolable_value));
      non_interpolable_values[i] =
          std::move(converted_side.non_interpolable_value);
    }
  }

  return InterpolationValue(
      std::move(list),
      CSSBorderImageLengthBoxNonInterpolableValue::Create(
          SideTypes(quad), std::move(non_interpolable_values)));
}

InterpolationValue CSSBorderImageLengthBoxInterpolationType::
    MaybeConvertStandardPropertyUnderlyingValue(
        const ComputedStyle& style) const {
  return ConvertBorderImageLengthBox(
      GetBorderImageLengthBox(CssProperty(), style), style.EffectiveZoom());
}

PairwiseInterpolationValue
CSSBorderImageLengthBoxInterpolationType::MaybeMergeSingles(
    InterpolationValue&& start,
    InterpolationValue&& end) const {
  const SideTypes& start_side_types =
      ToCSSBorderImageLengthBoxNonInterpolableValue(
          *start.non_interpolable_value)
          .GetSideTypes();
  const SideTypes& end_side_types =
      ToCSSBorderImageLengthBoxNonInterpolableValue(*end.non_interpolable_value)
          .GetSideTypes();
  if (start_side_types != end_side_types)
    return nullptr;

  return PairwiseInterpolationValue(std::move(start.interpolable_value),
                                    std::move(end.interpolable_value),
                                    std::move(start.non_interpolable_value));
}

void CSSBorderImageLengthBoxInterpolationType::Composite(
    UnderlyingValueOwner& underlying_value_owner,
    double underlying_fraction,
    const InterpolationValue& value,
    double interpolation_fraction) const {
  const SideTypes& underlying_side_types =
      ToCSSBorderImageLengthBoxNonInterpolableValue(
          *underlying_value_owner.Value().non_interpolable_value)
          .GetSideTypes();
  const auto& non_interpolable_value =
      ToCSSBorderImageLengthBoxNonInterpolableValue(
          *value.non_interpolable_value);
  const SideTypes& side_types = non_interpolable_value.GetSideTypes();

  if (underlying_side_types != side_types) {
    underlying_value_owner.Set(*this, value);
    return;
  }

  InterpolationValue& underlying_value = underlying_value_owner.MutableValue();
  InterpolableList& underlying_list =
      ToInterpolableList(*underlying_value.interpolable_value);
  Vector<scoped_refptr<NonInterpolableValue>>&
      underlying_side_non_interpolable_values =
          ToCSSBorderImageLengthBoxNonInterpolableValue(
              *underlying_value.non_interpolable_value)
              .SideNonInterpolableValues();
  const InterpolableList& list = ToInterpolableList(*value.interpolable_value);
  const Vector<scoped_refptr<NonInterpolableValue>>&
      side_non_interpolable_values =
          non_interpolable_value.SideNonInterpolableValues();

  for (size_t i = 0; i < kSideIndexCount; i++) {
    switch (side_types.type[i]) {
      case SideType::kNumber:
        underlying_list.GetMutable(i)->ScaleAndAdd(underlying_fraction,
                                                   *list.Get(i));
        break;
      case SideType::kLength:
        LengthInterpolationFunctions::Composite(
            underlying_list.GetMutable(i),
            underlying_side_non_interpolable_values[i], underlying_fraction,
            *list.Get(i), side_non_interpolable_values[i].get());
        break;
      case SideType::kAuto:
        break;
      default:
        NOTREACHED();
        break;
    }
  }
}

void CSSBorderImageLengthBoxInterpolationType::ApplyStandardPropertyValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue* non_interpolable_value,
    StyleResolverState& state) const {
  const SideTypes& side_types =
      ToCSSBorderImageLengthBoxNonInterpolableValue(non_interpolable_value)
          ->GetSideTypes();
  const Vector<scoped_refptr<NonInterpolableValue>>& non_interpolable_values =
      ToCSSBorderImageLengthBoxNonInterpolableValue(non_interpolable_value)
          ->SideNonInterpolableValues();
  const InterpolableList& list = ToInterpolableList(interpolable_value);
  const auto& convert_side =
      [&side_types, &list, &state,
       &non_interpolable_values](size_t index) -> BorderImageLength {
    switch (side_types.type[index]) {
      case SideType::kNumber:
        return clampTo<double>(ToInterpolableNumber(list.Get(index))->Value(),
                               0);
      case SideType::kAuto:
        return Length(kAuto);
      case SideType::kLength:
        return LengthInterpolationFunctions::CreateLength(
            *list.Get(index), non_interpolable_values[index].get(),
            state.CssToLengthConversionData(), kValueRangeNonNegative);
      default:
        NOTREACHED();
        return Length(kAuto);
    }
  };
  BorderImageLengthBox box(convert_side(kSideTop), convert_side(kSideRight),
                           convert_side(kSideBottom), convert_side(kSideLeft));
  SetBorderImageLengthBox(CssProperty(), *state.Style(), box);
}

}  // namespace blink

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css_basic_shape_interpolation_type.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/animation/basic_shape_interpolation_functions.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_state.h"
#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/core/style/basic_shapes.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/style/data_equivalency.h"

namespace blink {

namespace {

const BasicShape* GetBasicShape(const CSSProperty& property,
                                const ComputedStyle& style) {
  switch (property.PropertyID()) {
    case CSSPropertyShapeOutside:
      if (!style.ShapeOutside())
        return nullptr;
      if (style.ShapeOutside()->GetType() != ShapeValue::kShape)
        return nullptr;
      if (style.ShapeOutside()->CssBox() != CSSBoxType::kMissing)
        return nullptr;
      return style.ShapeOutside()->Shape();
    case CSSPropertyClipPath:
      if (!style.ClipPath())
        return nullptr;
      if (style.ClipPath()->GetType() != ClipPathOperation::SHAPE)
        return nullptr;
      return ToShapeClipPathOperation(style.ClipPath())->GetBasicShape();
    default:
      NOTREACHED();
      return nullptr;
  }
}

class UnderlyingCompatibilityChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  static std::unique_ptr<UnderlyingCompatibilityChecker> Create(
      scoped_refptr<NonInterpolableValue> underlying_non_interpolable_value) {
    return base::WrapUnique(new UnderlyingCompatibilityChecker(
        std::move(underlying_non_interpolable_value)));
  }

 private:
  UnderlyingCompatibilityChecker(
      scoped_refptr<NonInterpolableValue> underlying_non_interpolable_value)
      : underlying_non_interpolable_value_(
            std::move(underlying_non_interpolable_value)) {}

  bool IsValid(const StyleResolverState&,
               const InterpolationValue& underlying) const final {
    return BasicShapeInterpolationFunctions::ShapesAreCompatible(
        *underlying_non_interpolable_value_,
        *underlying.non_interpolable_value);
  }

  scoped_refptr<NonInterpolableValue> underlying_non_interpolable_value_;
};

class InheritedShapeChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  static std::unique_ptr<InheritedShapeChecker> Create(
      const CSSProperty& property,
      scoped_refptr<BasicShape> inherited_shape) {
    return base::WrapUnique(
        new InheritedShapeChecker(property, std::move(inherited_shape)));
  }

 private:
  InheritedShapeChecker(const CSSProperty& property,
                        scoped_refptr<BasicShape> inherited_shape)
      : property_(property), inherited_shape_(std::move(inherited_shape)) {}

  bool IsValid(const StyleResolverState& state,
               const InterpolationValue&) const final {
    return DataEquivalent(inherited_shape_.get(),
                          GetBasicShape(property_, *state.ParentStyle()));
  }

  const CSSProperty& property_;
  scoped_refptr<BasicShape> inherited_shape_;
};

}  // namespace

InterpolationValue CSSBasicShapeInterpolationType::MaybeConvertNeutral(
    const InterpolationValue& underlying,
    ConversionCheckers& conversion_checkers) const {
  // const_cast is for taking refs.
  NonInterpolableValue* non_interpolable_value =
      const_cast<NonInterpolableValue*>(
          underlying.non_interpolable_value.get());
  conversion_checkers.push_back(
      UnderlyingCompatibilityChecker::Create(non_interpolable_value));
  return InterpolationValue(
      BasicShapeInterpolationFunctions::CreateNeutralValue(
          *underlying.non_interpolable_value),
      non_interpolable_value);
}

InterpolationValue CSSBasicShapeInterpolationType::MaybeConvertInitial(
    const StyleResolverState&,
    ConversionCheckers&) const {
  return BasicShapeInterpolationFunctions::MaybeConvertBasicShape(
      GetBasicShape(CssProperty(), ComputedStyle::InitialStyle()), 1);
}

InterpolationValue CSSBasicShapeInterpolationType::MaybeConvertInherit(
    const StyleResolverState& state,
    ConversionCheckers& conversion_checkers) const {
  const BasicShape* shape = GetBasicShape(CssProperty(), *state.ParentStyle());
  // const_cast to take a ref.
  conversion_checkers.push_back(InheritedShapeChecker::Create(
      CssProperty(), const_cast<BasicShape*>(shape)));
  return BasicShapeInterpolationFunctions::MaybeConvertBasicShape(
      shape, state.ParentStyle()->EffectiveZoom());
}

InterpolationValue CSSBasicShapeInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState*,
    ConversionCheckers&) const {
  if (!value.IsBaseValueList())
    return BasicShapeInterpolationFunctions::MaybeConvertCSSValue(value);

  const CSSValueList& list = ToCSSValueList(value);
  if (list.length() != 1)
    return nullptr;
  return BasicShapeInterpolationFunctions::MaybeConvertCSSValue(list.Item(0));
}

PairwiseInterpolationValue CSSBasicShapeInterpolationType::MaybeMergeSingles(
    InterpolationValue&& start,
    InterpolationValue&& end) const {
  if (!BasicShapeInterpolationFunctions::ShapesAreCompatible(
          *start.non_interpolable_value, *end.non_interpolable_value))
    return nullptr;
  return PairwiseInterpolationValue(std::move(start.interpolable_value),
                                    std::move(end.interpolable_value),
                                    std::move(start.non_interpolable_value));
}

InterpolationValue
CSSBasicShapeInterpolationType::MaybeConvertStandardPropertyUnderlyingValue(
    const ComputedStyle& style) const {
  return BasicShapeInterpolationFunctions::MaybeConvertBasicShape(
      GetBasicShape(CssProperty(), style), style.EffectiveZoom());
}

void CSSBasicShapeInterpolationType::Composite(
    UnderlyingValueOwner& underlying_value_owner,
    double underlying_fraction,
    const InterpolationValue& value,
    double interpolation_fraction) const {
  if (!BasicShapeInterpolationFunctions::ShapesAreCompatible(
          *underlying_value_owner.Value().non_interpolable_value,
          *value.non_interpolable_value)) {
    underlying_value_owner.Set(*this, value);
    return;
  }

  underlying_value_owner.MutableValue().interpolable_value->ScaleAndAdd(
      underlying_fraction, *value.interpolable_value);
}

void CSSBasicShapeInterpolationType::ApplyStandardPropertyValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue* non_interpolable_value,
    StyleResolverState& state) const {
  scoped_refptr<BasicShape> shape =
      BasicShapeInterpolationFunctions::CreateBasicShape(
          interpolable_value, *non_interpolable_value,
          state.CssToLengthConversionData());
  switch (CssProperty().PropertyID()) {
    case CSSPropertyShapeOutside:
      state.Style()->SetShapeOutside(
          ShapeValue::CreateShapeValue(std::move(shape), CSSBoxType::kMissing));
      break;
    case CSSPropertyClipPath:
      state.Style()->SetClipPath(
          ShapeClipPathOperation::Create(std::move(shape)));
      break;
    default:
      NOTREACHED();
      break;
  }
}

}  // namespace blink

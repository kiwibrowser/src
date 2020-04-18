// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css_transform_interpolation_type.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/animation/length_units_checker.h"
#include "third_party/blink/renderer/core/css/css_function_value.h"
#include "third_party/blink/renderer/core/css/css_primitive_value.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_state.h"
#include "third_party/blink/renderer/core/css/resolver/transform_builder.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/transforms/transform_operations.h"
#include "third_party/blink/renderer/platform/transforms/translate_transform_operation.h"

namespace blink {

class CSSTransformNonInterpolableValue : public NonInterpolableValue {
 public:
  static scoped_refptr<CSSTransformNonInterpolableValue> Create(
      TransformOperations&& transform) {
    return base::AdoptRef(new CSSTransformNonInterpolableValue(
        true, std::move(transform), EmptyTransformOperations(), false, false));
  }

  static scoped_refptr<CSSTransformNonInterpolableValue> Create(
      CSSTransformNonInterpolableValue&& start,
      double start_fraction,
      CSSTransformNonInterpolableValue&& end,
      double end_fraction) {
    return base::AdoptRef(new CSSTransformNonInterpolableValue(
        false, start.GetInterpolatedTransform(start_fraction),
        end.GetInterpolatedTransform(end_fraction), start.IsAdditive(),
        end.IsAdditive()));
  }

  scoped_refptr<CSSTransformNonInterpolableValue> Composite(
      const CSSTransformNonInterpolableValue& other,
      double other_progress) {
    DCHECK(!IsAdditive());
    if (other.is_single_) {
      DCHECK_EQ(other_progress, 0);
      DCHECK(other.IsAdditive());
      TransformOperations result;
      result.Operations() = Concat(Transform(), other.Transform());
      return Create(std::move(result));
    }

    DCHECK(other.is_start_additive_ || other.is_end_additive_);
    TransformOperations start;
    start.Operations() = other.is_start_additive_
                             ? Concat(Transform(), other.start_)
                             : other.start_.Operations();
    TransformOperations end;
    end.Operations() = other.is_end_additive_ ? Concat(Transform(), other.end_)
                                              : other.end_.Operations();
    return Create(end.Blend(start, other_progress));
  }

  void SetSingleAdditive() {
    DCHECK(is_single_);
    is_start_additive_ = true;
    is_end_additive_ = true;
  }

  TransformOperations GetInterpolatedTransform(double progress) const {
    if (progress == 0)
      return start_;
    if (progress == 1)
      return end_;
    DCHECK(!IsAdditive());
    return end_.Blend(start_, progress);
  }

  DECLARE_NON_INTERPOLABLE_VALUE_TYPE();

 private:
  CSSTransformNonInterpolableValue(bool is_single,
                                   TransformOperations&& start,
                                   TransformOperations&& end,
                                   bool is_start_additive,
                                   bool is_end_additive)
      : is_single_(is_single),
        start_(std::move(start)),
        end_(std::move(end)),
        is_start_additive_(is_start_additive),
        is_end_additive_(is_end_additive) {}

  const TransformOperations& Transform() const {
    DCHECK(is_single_);
    return start_;
  }
  bool IsAdditive() const {
    bool result = is_start_additive_ || is_end_additive_;
    DCHECK(!result || is_single_);
    return result;
  }

  Vector<scoped_refptr<TransformOperation>> Concat(
      const TransformOperations& a,
      const TransformOperations& b) {
    Vector<scoped_refptr<TransformOperation>> result;
    result.ReserveCapacity(a.size() + b.size());
    result.AppendVector(a.Operations());
    result.AppendVector(b.Operations());
    return result;
  }

  bool is_single_;
  TransformOperations start_;
  TransformOperations end_;
  bool is_start_additive_;
  bool is_end_additive_;
};

DEFINE_NON_INTERPOLABLE_VALUE_TYPE(CSSTransformNonInterpolableValue);
DEFINE_NON_INTERPOLABLE_VALUE_TYPE_CASTS(CSSTransformNonInterpolableValue);

namespace {

InterpolationValue ConvertTransform(TransformOperations&& transform) {
  return InterpolationValue(
      InterpolableNumber::Create(0),
      CSSTransformNonInterpolableValue::Create(std::move(transform)));
}

InterpolationValue ConvertTransform(const TransformOperations& transform) {
  return ConvertTransform(TransformOperations(transform));
}

class InheritedTransformChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  static std::unique_ptr<InheritedTransformChecker> Create(
      const TransformOperations& inherited_transform) {
    return base::WrapUnique(new InheritedTransformChecker(inherited_transform));
  }

  bool IsValid(const StyleResolverState& state,
               const InterpolationValue& underlying) const final {
    return inherited_transform_ == state.ParentStyle()->Transform();
  }

 private:
  InheritedTransformChecker(const TransformOperations& inherited_transform)
      : inherited_transform_(inherited_transform) {}

  const TransformOperations inherited_transform_;
};

}  // namespace

InterpolationValue CSSTransformInterpolationType::MaybeConvertNeutral(
    const InterpolationValue& underlying,
    ConversionCheckers&) const {
  return ConvertTransform(EmptyTransformOperations());
}

InterpolationValue CSSTransformInterpolationType::MaybeConvertInitial(
    const StyleResolverState&,
    ConversionCheckers&) const {
  return ConvertTransform(ComputedStyle::InitialStyle().Transform());
}

InterpolationValue CSSTransformInterpolationType::MaybeConvertInherit(
    const StyleResolverState& state,
    ConversionCheckers& conversion_checkers) const {
  const TransformOperations& inherited_transform =
      state.ParentStyle()->Transform();
  conversion_checkers.push_back(
      InheritedTransformChecker::Create(inherited_transform));
  return ConvertTransform(inherited_transform);
}

InterpolationValue CSSTransformInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState* state,
    ConversionCheckers& conversion_checkers) const {
  DCHECK(state);
  if (value.IsValueList()) {
    CSSLengthArray length_array;
    for (const CSSValue* item : ToCSSValueList(value)) {
      const CSSFunctionValue& transform_function = ToCSSFunctionValue(*item);
      if (transform_function.FunctionType() == CSSValueMatrix ||
          transform_function.FunctionType() == CSSValueMatrix3d) {
        length_array.type_flags.Set(CSSPrimitiveValue::kUnitTypePixels);
        continue;
      }
      for (const CSSValue* argument : transform_function) {
        const CSSPrimitiveValue& primitive_value =
            ToCSSPrimitiveValue(*argument);
        if (!primitive_value.IsLength())
          continue;
        primitive_value.AccumulateLengthArray(length_array);
      }
    }
    std::unique_ptr<InterpolationType::ConversionChecker> length_units_checker =
        LengthUnitsChecker::MaybeCreate(std::move(length_array), *state);

    if (length_units_checker)
      conversion_checkers.push_back(std::move(length_units_checker));
  }

  DCHECK(state);
  TransformOperations transform = TransformBuilder::CreateTransformOperations(
      value, state->CssToLengthConversionData());
  return ConvertTransform(std::move(transform));
}

void CSSTransformInterpolationType::AdditiveKeyframeHook(
    InterpolationValue& value) const {
  ToCSSTransformNonInterpolableValue(*value.non_interpolable_value)
      .SetSingleAdditive();
}

PairwiseInterpolationValue CSSTransformInterpolationType::MaybeMergeSingles(
    InterpolationValue&& start,
    InterpolationValue&& end) const {
  double start_fraction =
      ToInterpolableNumber(*start.interpolable_value).Value();
  double end_fraction = ToInterpolableNumber(*end.interpolable_value).Value();
  return PairwiseInterpolationValue(
      InterpolableNumber::Create(0), InterpolableNumber::Create(1),
      CSSTransformNonInterpolableValue::Create(
          std::move(ToCSSTransformNonInterpolableValue(
              *start.non_interpolable_value)),
          start_fraction,
          std::move(
              ToCSSTransformNonInterpolableValue(*end.non_interpolable_value)),
          end_fraction));
}

InterpolationValue
CSSTransformInterpolationType::MaybeConvertStandardPropertyUnderlyingValue(
    const ComputedStyle& style) const {
  return ConvertTransform(style.Transform());
}

void CSSTransformInterpolationType::Composite(
    UnderlyingValueOwner& underlying_value_owner,
    double underlying_fraction,
    const InterpolationValue& value,
    double interpolation_fraction) const {
  CSSTransformNonInterpolableValue& underlying_non_interpolable_value =
      ToCSSTransformNonInterpolableValue(
          *underlying_value_owner.Value().non_interpolable_value);
  const CSSTransformNonInterpolableValue& non_interpolable_value =
      ToCSSTransformNonInterpolableValue(*value.non_interpolable_value);
  double progress = ToInterpolableNumber(*value.interpolable_value).Value();
  underlying_value_owner.MutableValue().non_interpolable_value =
      underlying_non_interpolable_value.Composite(non_interpolable_value,
                                                  progress);
}

void CSSTransformInterpolationType::ApplyStandardPropertyValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue* untyped_non_interpolable_value,
    StyleResolverState& state) const {
  double progress = ToInterpolableNumber(interpolable_value).Value();
  const CSSTransformNonInterpolableValue& non_interpolable_value =
      ToCSSTransformNonInterpolableValue(*untyped_non_interpolable_value);
  state.Style()->SetTransform(
      non_interpolable_value.GetInterpolatedTransform(progress));
}

}  // namespace blink

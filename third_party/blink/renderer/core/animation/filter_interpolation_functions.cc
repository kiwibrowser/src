// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/filter_interpolation_functions.h"

#include <memory>
#include "third_party/blink/renderer/core/animation/length_interpolation_functions.h"
#include "third_party/blink/renderer/core/animation/shadow_interpolation_functions.h"
#include "third_party/blink/renderer/core/css/css_function_value.h"
#include "third_party/blink/renderer/core/css/css_primitive_value.h"
#include "third_party/blink/renderer/core/css/resolver/filter_operation_resolver.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_state.h"
#include "third_party/blink/renderer/core/style/filter_operations.h"
#include "third_party/blink/renderer/core/style/shadow_data.h"

namespace blink {

class FilterNonInterpolableValue : public NonInterpolableValue {
 public:
  static scoped_refptr<FilterNonInterpolableValue> Create(
      FilterOperation::OperationType type,
      scoped_refptr<NonInterpolableValue> type_non_interpolable_value) {
    return base::AdoptRef(new FilterNonInterpolableValue(
        type, std::move(type_non_interpolable_value)));
  }

  FilterOperation::OperationType GetOperationType() const { return type_; }
  const NonInterpolableValue* TypeNonInterpolableValue() const {
    return type_non_interpolable_value_.get();
  }

  DECLARE_NON_INTERPOLABLE_VALUE_TYPE();

 private:
  FilterNonInterpolableValue(
      FilterOperation::OperationType type,
      scoped_refptr<NonInterpolableValue> type_non_interpolable_value)
      : type_(type),
        type_non_interpolable_value_(std::move(type_non_interpolable_value)) {}

  const FilterOperation::OperationType type_;
  scoped_refptr<NonInterpolableValue> type_non_interpolable_value_;
};

DEFINE_NON_INTERPOLABLE_VALUE_TYPE(FilterNonInterpolableValue);
DEFINE_NON_INTERPOLABLE_VALUE_TYPE_CASTS(FilterNonInterpolableValue);

namespace {

double DefaultParameter(FilterOperation::OperationType type) {
  switch (type) {
    case FilterOperation::BRIGHTNESS:
    case FilterOperation::GRAYSCALE:
    case FilterOperation::SATURATE:
    case FilterOperation::SEPIA:
      return 1;

    case FilterOperation::CONTRAST:
    case FilterOperation::HUE_ROTATE:
    case FilterOperation::INVERT:
    case FilterOperation::OPACITY:
      return 0;

    default:
      NOTREACHED();
      return 0;
  }
}

double ClampParameter(double value, FilterOperation::OperationType type) {
  switch (type) {
    case FilterOperation::BRIGHTNESS:
    case FilterOperation::CONTRAST:
    case FilterOperation::SATURATE:
      return clampTo<double>(value, 0);

    case FilterOperation::GRAYSCALE:
    case FilterOperation::INVERT:
    case FilterOperation::OPACITY:
    case FilterOperation::SEPIA:
      return clampTo<double>(value, 0, 1);

    case FilterOperation::HUE_ROTATE:
      return value;

    default:
      NOTREACHED();
      return 0;
  }
}

}  // namespace

InterpolationValue FilterInterpolationFunctions::MaybeConvertCSSFilter(
    const CSSValue& value) {
  if (value.IsURIValue())
    return nullptr;

  const CSSFunctionValue& filter = ToCSSFunctionValue(value);
  DCHECK_LE(filter.length(), 1u);
  FilterOperation::OperationType type =
      FilterOperationResolver::FilterOperationForType(filter.FunctionType());
  InterpolationValue result = nullptr;

  switch (type) {
    case FilterOperation::BRIGHTNESS:
    case FilterOperation::CONTRAST:
    case FilterOperation::GRAYSCALE:
    case FilterOperation::INVERT:
    case FilterOperation::OPACITY:
    case FilterOperation::SATURATE:
    case FilterOperation::SEPIA: {
      double amount = DefaultParameter(type);
      if (filter.length() == 1) {
        const CSSPrimitiveValue& first_value =
            ToCSSPrimitiveValue(filter.Item(0));
        amount = first_value.GetDoubleValue();
        if (first_value.IsPercentage())
          amount /= 100;
      }
      result.interpolable_value = InterpolableNumber::Create(amount);
      break;
    }

    case FilterOperation::HUE_ROTATE: {
      double angle = DefaultParameter(type);
      if (filter.length() == 1)
        angle = ToCSSPrimitiveValue(filter.Item(0)).ComputeDegrees();
      result.interpolable_value = InterpolableNumber::Create(angle);
      break;
    }

    case FilterOperation::BLUR: {
      if (filter.length() == 0)
        result.interpolable_value =
            LengthInterpolationFunctions::CreateNeutralInterpolableValue();
      else
        result =
            LengthInterpolationFunctions::MaybeConvertCSSValue(filter.Item(0));
      break;
    }

    case FilterOperation::DROP_SHADOW: {
      result =
          ShadowInterpolationFunctions::MaybeConvertCSSValue(filter.Item(0));
      break;
    }

    default:
      NOTREACHED();
      return nullptr;
  }

  if (!result)
    return nullptr;

  result.non_interpolable_value = FilterNonInterpolableValue::Create(
      type, std::move(result.non_interpolable_value));
  return result;
}

InterpolationValue FilterInterpolationFunctions::MaybeConvertFilter(
    const FilterOperation& filter,
    double zoom) {
  InterpolationValue result = nullptr;

  switch (filter.GetType()) {
    case FilterOperation::GRAYSCALE:
    case FilterOperation::HUE_ROTATE:
    case FilterOperation::SATURATE:
    case FilterOperation::SEPIA:
      result.interpolable_value = InterpolableNumber::Create(
          ToBasicColorMatrixFilterOperation(filter).Amount());
      break;

    case FilterOperation::BRIGHTNESS:
    case FilterOperation::CONTRAST:
    case FilterOperation::INVERT:
    case FilterOperation::OPACITY:
      result.interpolable_value = InterpolableNumber::Create(
          ToBasicComponentTransferFilterOperation(filter).Amount());
      break;

    case FilterOperation::BLUR:
      result = LengthInterpolationFunctions::MaybeConvertLength(
          ToBlurFilterOperation(filter).StdDeviation(), zoom);
      break;

    case FilterOperation::DROP_SHADOW: {
      result = ShadowInterpolationFunctions::ConvertShadowData(
          ToDropShadowFilterOperation(filter).Shadow(), zoom);
      break;
    }

    case FilterOperation::REFERENCE:
      return nullptr;

    default:
      NOTREACHED();
      return nullptr;
  }

  if (!result)
    return nullptr;

  result.non_interpolable_value = FilterNonInterpolableValue::Create(
      filter.GetType(), std::move(result.non_interpolable_value));
  return result;
}

std::unique_ptr<InterpolableValue>
FilterInterpolationFunctions::CreateNoneValue(
    const NonInterpolableValue& untyped_value) {
  switch (ToFilterNonInterpolableValue(untyped_value).GetOperationType()) {
    case FilterOperation::GRAYSCALE:
    case FilterOperation::INVERT:
    case FilterOperation::SEPIA:
    case FilterOperation::HUE_ROTATE:
      return InterpolableNumber::Create(0);

    case FilterOperation::BRIGHTNESS:
    case FilterOperation::CONTRAST:
    case FilterOperation::OPACITY:
    case FilterOperation::SATURATE:
      return InterpolableNumber::Create(1);

    case FilterOperation::BLUR:
      return LengthInterpolationFunctions::CreateNeutralInterpolableValue();

    case FilterOperation::DROP_SHADOW:
      return ShadowInterpolationFunctions::CreateNeutralInterpolableValue();

    default:
      NOTREACHED();
      return nullptr;
  }
}

bool FilterInterpolationFunctions::FiltersAreCompatible(
    const NonInterpolableValue& a,
    const NonInterpolableValue& b) {
  return ToFilterNonInterpolableValue(a).GetOperationType() ==
         ToFilterNonInterpolableValue(b).GetOperationType();
}

FilterOperation* FilterInterpolationFunctions::CreateFilter(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue& untyped_non_interpolable_value,
    const StyleResolverState& state) {
  const FilterNonInterpolableValue& non_interpolable_value =
      ToFilterNonInterpolableValue(untyped_non_interpolable_value);
  FilterOperation::OperationType type =
      non_interpolable_value.GetOperationType();

  switch (type) {
    case FilterOperation::GRAYSCALE:
    case FilterOperation::HUE_ROTATE:
    case FilterOperation::SATURATE:
    case FilterOperation::SEPIA: {
      double value = ClampParameter(
          ToInterpolableNumber(interpolable_value).Value(), type);
      return BasicColorMatrixFilterOperation::Create(value, type);
    }

    case FilterOperation::BRIGHTNESS:
    case FilterOperation::CONTRAST:
    case FilterOperation::INVERT:
    case FilterOperation::OPACITY: {
      double value = ClampParameter(
          ToInterpolableNumber(interpolable_value).Value(), type);
      return BasicComponentTransferFilterOperation::Create(value, type);
    }

    case FilterOperation::BLUR: {
      Length std_deviation = LengthInterpolationFunctions::CreateLength(
          interpolable_value, non_interpolable_value.TypeNonInterpolableValue(),
          state.CssToLengthConversionData(), kValueRangeNonNegative);
      return BlurFilterOperation::Create(std_deviation);
    }

    case FilterOperation::DROP_SHADOW: {
      ShadowData shadow_data = ShadowInterpolationFunctions::CreateShadowData(
          interpolable_value, non_interpolable_value.TypeNonInterpolableValue(),
          state);
      if (shadow_data.GetColor().IsCurrentColor())
        shadow_data.OverrideColor(Color::kBlack);
      return DropShadowFilterOperation::Create(shadow_data);
    }

    default:
      NOTREACHED();
      return nullptr;
  }
}

}  // namespace blink

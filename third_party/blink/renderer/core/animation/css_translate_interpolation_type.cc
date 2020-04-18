// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css_translate_interpolation_type.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/animation/length_interpolation_functions.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_state.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/transforms/translate_transform_operation.h"

namespace blink {

namespace {

InterpolationValue CreateNoneValue() {
  return InterpolationValue(InterpolableList::Create(0));
}

bool IsNoneValue(const InterpolationValue& value) {
  return ToInterpolableList(*value.interpolable_value).length() == 0;
}

class InheritedTranslateChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  ~InheritedTranslateChecker() override = default;

  static std::unique_ptr<InheritedTranslateChecker> Create(
      scoped_refptr<TranslateTransformOperation> inherited_translate) {
    return base::WrapUnique(
        new InheritedTranslateChecker(std::move(inherited_translate)));
  }

  bool IsValid(const StyleResolverState& state,
               const InterpolationValue& underlying) const final {
    const TransformOperation* inherited_translate =
        state.ParentStyle()->Translate();
    if (inherited_translate_ == inherited_translate)
      return true;
    if (!inherited_translate_ || !inherited_translate)
      return false;
    return *inherited_translate_ == *inherited_translate;
  }

 private:
  InheritedTranslateChecker(
      scoped_refptr<TranslateTransformOperation> inherited_translate)
      : inherited_translate_(std::move(inherited_translate)) {}

  scoped_refptr<TransformOperation> inherited_translate_;
};

enum TranslateComponentIndex : unsigned {
  kTranslateX,
  kTranslateY,
  kTranslateZ,
  kTranslateComponentIndexCount,
};

std::unique_ptr<InterpolableValue> CreateTranslateIdentity() {
  std::unique_ptr<InterpolableList> result =
      InterpolableList::Create(kTranslateComponentIndexCount);
  result->Set(kTranslateX,
              LengthInterpolationFunctions::CreateNeutralInterpolableValue());
  result->Set(kTranslateY,
              LengthInterpolationFunctions::CreateNeutralInterpolableValue());
  result->Set(kTranslateZ,
              LengthInterpolationFunctions::CreateNeutralInterpolableValue());
  return std::move(result);
}

InterpolationValue ConvertTranslateOperation(
    const TranslateTransformOperation* translate,
    double zoom) {
  if (!translate)
    return CreateNoneValue();

  std::unique_ptr<InterpolableList> result =
      InterpolableList::Create(kTranslateComponentIndexCount);
  result->Set(kTranslateX, LengthInterpolationFunctions::MaybeConvertLength(
                               translate->X(), zoom)
                               .interpolable_value);
  result->Set(kTranslateY, LengthInterpolationFunctions::MaybeConvertLength(
                               translate->Y(), zoom)
                               .interpolable_value);
  result->Set(kTranslateZ, LengthInterpolationFunctions::MaybeConvertLength(
                               Length(translate->Z(), kFixed), zoom)
                               .interpolable_value);
  return InterpolationValue(std::move(result));
}

}  // namespace

InterpolationValue CSSTranslateInterpolationType::MaybeConvertNeutral(
    const InterpolationValue& underlying,
    ConversionCheckers&) const {
  return InterpolationValue(CreateTranslateIdentity());
}

InterpolationValue CSSTranslateInterpolationType::MaybeConvertInitial(
    const StyleResolverState&,
    ConversionCheckers&) const {
  return CreateNoneValue();
}

InterpolationValue CSSTranslateInterpolationType::MaybeConvertInherit(
    const StyleResolverState& state,
    ConversionCheckers& conversion_checkers) const {
  TranslateTransformOperation* inherited_translate =
      state.ParentStyle()->Translate();
  conversion_checkers.push_back(
      InheritedTranslateChecker::Create(inherited_translate));
  return ConvertTranslateOperation(inherited_translate,
                                   state.ParentStyle()->EffectiveZoom());
}

InterpolationValue CSSTranslateInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState*,
    ConversionCheckers&) const {
  if (!value.IsBaseValueList()) {
    return CreateNoneValue();
  }

  const CSSValueList& list = ToCSSValueList(value);
  if (list.length() < 1 || list.length() > 3)
    return nullptr;

  std::unique_ptr<InterpolableList> result =
      InterpolableList::Create(kTranslateComponentIndexCount);
  for (size_t i = 0; i < kTranslateComponentIndexCount; i++) {
    InterpolationValue component = nullptr;
    if (i < list.length()) {
      component =
          LengthInterpolationFunctions::MaybeConvertCSSValue(list.Item(i));
      if (!component)
        return nullptr;
    } else {
      component = InterpolationValue(
          LengthInterpolationFunctions::CreateNeutralInterpolableValue());
    }
    result->Set(i, std::move(component.interpolable_value));
  }
  return InterpolationValue(std::move(result));
}

PairwiseInterpolationValue CSSTranslateInterpolationType::MaybeMergeSingles(
    InterpolationValue&& start,
    InterpolationValue&& end) const {
  size_t start_list_length =
      ToInterpolableList(*start.interpolable_value).length();
  size_t end_list_length = ToInterpolableList(*end.interpolable_value).length();
  if (start_list_length < end_list_length)
    start.interpolable_value = CreateTranslateIdentity();
  else if (end_list_length < start_list_length)
    end.interpolable_value = CreateTranslateIdentity();

  return PairwiseInterpolationValue(std::move(start.interpolable_value),
                                    std::move(end.interpolable_value));
}

InterpolationValue
CSSTranslateInterpolationType::MaybeConvertStandardPropertyUnderlyingValue(
    const ComputedStyle& style) const {
  return ConvertTranslateOperation(style.Translate(), style.EffectiveZoom());
}

void CSSTranslateInterpolationType::Composite(
    UnderlyingValueOwner& underlying_value_owner,
    double underlying_fraction,
    const InterpolationValue& value,
    double interpolation_fraction) const {
  if (IsNoneValue(value)) {
    return;
  }

  if (IsNoneValue(underlying_value_owner.MutableValue())) {
    underlying_value_owner.MutableValue().interpolable_value =
        CreateTranslateIdentity();
  }

  return CSSInterpolationType::Composite(underlying_value_owner,
                                         underlying_fraction, value,
                                         interpolation_fraction);
}

void CSSTranslateInterpolationType::ApplyStandardPropertyValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue*,
    StyleResolverState& state) const {
  const InterpolableList& list = ToInterpolableList(interpolable_value);
  if (list.length() == 0) {
    state.Style()->SetTranslate(nullptr);
    return;
  }
  const CSSToLengthConversionData& conversion_data =
      state.CssToLengthConversionData();
  Length x = LengthInterpolationFunctions::CreateLength(
      *list.Get(kTranslateX), nullptr, conversion_data, kValueRangeAll);
  Length y = LengthInterpolationFunctions::CreateLength(
      *list.Get(kTranslateY), nullptr, conversion_data, kValueRangeAll);
  float z =
      LengthInterpolationFunctions::CreateLength(
          *list.Get(kTranslateZ), nullptr, conversion_data, kValueRangeAll)
          .Pixels();

  scoped_refptr<TranslateTransformOperation> result =
      TranslateTransformOperation::Create(x, y, z,
                                          TransformOperation::kTranslate3D);
  state.Style()->SetTranslate(std::move(result));
}

}  // namespace blink

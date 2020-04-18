// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "third_party/blink/renderer/core/animation/svg_rect_interpolation_type.h"

#include <memory>
#include "third_party/blink/renderer/core/animation/interpolation_environment.h"
#include "third_party/blink/renderer/core/animation/string_keyframe.h"
#include "third_party/blink/renderer/core/svg/svg_rect.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"

namespace blink {

enum RectComponentIndex : unsigned {
  kRectX,
  kRectY,
  kRectWidth,
  kRectHeight,
  kRectComponentIndexCount,
};

InterpolationValue SVGRectInterpolationType::MaybeConvertNeutral(
    const InterpolationValue&,
    ConversionCheckers&) const {
  std::unique_ptr<InterpolableList> result =
      InterpolableList::Create(kRectComponentIndexCount);
  for (size_t i = 0; i < kRectComponentIndexCount; i++)
    result->Set(i, InterpolableNumber::Create(0));
  return InterpolationValue(std::move(result));
}

InterpolationValue SVGRectInterpolationType::MaybeConvertSVGValue(
    const SVGPropertyBase& svg_value) const {
  if (svg_value.GetType() != kAnimatedRect)
    return nullptr;

  const SVGRect& rect = ToSVGRect(svg_value);
  std::unique_ptr<InterpolableList> result =
      InterpolableList::Create(kRectComponentIndexCount);
  result->Set(kRectX, InterpolableNumber::Create(rect.X()));
  result->Set(kRectY, InterpolableNumber::Create(rect.Y()));
  result->Set(kRectWidth, InterpolableNumber::Create(rect.Width()));
  result->Set(kRectHeight, InterpolableNumber::Create(rect.Height()));
  return InterpolationValue(std::move(result));
}

SVGPropertyBase* SVGRectInterpolationType::AppliedSVGValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue*) const {
  const InterpolableList& list = ToInterpolableList(interpolable_value);
  SVGRect* result = SVGRect::Create();
  result->SetX(ToInterpolableNumber(list.Get(kRectX))->Value());
  result->SetY(ToInterpolableNumber(list.Get(kRectY))->Value());
  result->SetWidth(ToInterpolableNumber(list.Get(kRectWidth))->Value());
  result->SetHeight(ToInterpolableNumber(list.Get(kRectHeight))->Value());
  return result;
}

}  // namespace blink

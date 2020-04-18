// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_CSS_TRANSFORM_ORIGIN_INTERPOLATION_TYPE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_CSS_TRANSFORM_ORIGIN_INTERPOLATION_TYPE_H_

#include "third_party/blink/renderer/core/animation/css_length_list_interpolation_type.h"
#include "third_party/blink/renderer/core/animation/css_position_axis_list_interpolation_type.h"
#include "third_party/blink/renderer/core/animation/length_interpolation_functions.h"
#include "third_party/blink/renderer/core/animation/list_interpolation_functions.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"

namespace blink {

class CSSTransformOriginInterpolationType
    : public CSSLengthListInterpolationType {
 public:
  CSSTransformOriginInterpolationType(PropertyHandle property)
      : CSSLengthListInterpolationType(property) {}

 private:
  InterpolationValue MaybeConvertValue(const CSSValue& value,
                                       const StyleResolverState*,
                                       ConversionCheckers&) const final {
    const CSSValueList& list = ToCSSValueList(value);
    DCHECK_EQ(list.length(), 3U);
    return ListInterpolationFunctions::CreateList(
        list.length(), [&list](size_t index) {
          const CSSValue& item = list.Item(index);
          if (index < 2)
            return CSSPositionAxisListInterpolationType::
                ConvertPositionAxisCSSValue(item);
          return LengthInterpolationFunctions::MaybeConvertCSSValue(item);
        });
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_CSS_TRANSFORM_ORIGIN_INTERPOLATION_TYPE_H_

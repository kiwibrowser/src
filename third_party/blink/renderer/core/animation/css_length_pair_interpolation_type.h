// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_CSS_LENGTH_PAIR_INTERPOLATION_TYPE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_CSS_LENGTH_PAIR_INTERPOLATION_TYPE_H_

#include "third_party/blink/renderer/core/animation/css_length_list_interpolation_type.h"
#include "third_party/blink/renderer/core/animation/length_interpolation_functions.h"
#include "third_party/blink/renderer/core/animation/list_interpolation_functions.h"
#include "third_party/blink/renderer/core/css/css_value_pair.h"

namespace blink {

class CSSLengthPairInterpolationType : public CSSLengthListInterpolationType {
 public:
  CSSLengthPairInterpolationType(PropertyHandle property)
      : CSSLengthListInterpolationType(property) {}

 private:
  InterpolationValue MaybeConvertValue(const CSSValue& value,
                                       const StyleResolverState*,
                                       ConversionCheckers&) const final {
    const CSSValuePair& pair = ToCSSValuePair(value);
    return ListInterpolationFunctions::CreateList(2, [&pair](size_t index) {
      const CSSValue& item = index == 0 ? pair.First() : pair.Second();
      return LengthInterpolationFunctions::MaybeConvertCSSValue(item);
    });
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_CSS_LENGTH_PAIR_INTERPOLATION_TYPE_H_

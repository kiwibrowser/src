// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_SHADOW_INTERPOLATION_FUNCTIONS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_SHADOW_INTERPOLATION_FUNCTIONS_H_

#include <memory>
#include "third_party/blink/renderer/core/animation/interpolation_value.h"
#include "third_party/blink/renderer/core/animation/pairwise_interpolation_value.h"

namespace blink {

class ShadowData;
class CSSValue;
class StyleResolverState;

class ShadowInterpolationFunctions {
 public:
  static InterpolationValue ConvertShadowData(const ShadowData&, double zoom);
  static InterpolationValue MaybeConvertCSSValue(const CSSValue&);
  static std::unique_ptr<InterpolableValue> CreateNeutralInterpolableValue();
  static bool NonInterpolableValuesAreCompatible(const NonInterpolableValue*,
                                                 const NonInterpolableValue*);
  static PairwiseInterpolationValue MaybeMergeSingles(
      InterpolationValue&& start,
      InterpolationValue&& end);
  static void Composite(std::unique_ptr<InterpolableValue>&,
                        scoped_refptr<NonInterpolableValue>&,
                        double underlying_fraction,
                        const InterpolableValue&,
                        const NonInterpolableValue*);
  static ShadowData CreateShadowData(const InterpolableValue&,
                                     const NonInterpolableValue*,
                                     const StyleResolverState&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_SHADOW_INTERPOLATION_FUNCTIONS_H_

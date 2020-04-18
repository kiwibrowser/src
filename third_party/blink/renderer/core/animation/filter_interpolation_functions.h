// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_FILTER_INTERPOLATION_FUNCTIONS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_FILTER_INTERPOLATION_FUNCTIONS_H_

#include <memory>
#include "third_party/blink/renderer/core/animation/interpolation_value.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class FilterOperation;
class CSSValue;
class StyleResolverState;

namespace FilterInterpolationFunctions {

InterpolationValue MaybeConvertCSSFilter(const CSSValue&);
InterpolationValue MaybeConvertFilter(const FilterOperation&, double zoom);
std::unique_ptr<InterpolableValue> CreateNoneValue(const NonInterpolableValue&);
bool FiltersAreCompatible(const NonInterpolableValue&,
                          const NonInterpolableValue&);
FilterOperation* CreateFilter(const InterpolableValue&,
                              const NonInterpolableValue&,
                              const StyleResolverState&);

}  // namespace FilterInterpolationFunctions

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_FILTER_INTERPOLATION_FUNCTIONS_H_

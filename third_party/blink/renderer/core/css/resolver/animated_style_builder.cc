/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/css/resolver/animated_style_builder.h"

#include "third_party/blink/renderer/core/animation/animatable/animatable_double.h"
#include "third_party/blink/renderer/core/animation/animatable/animatable_filter_operations.h"
#include "third_party/blink/renderer/core/animation/animatable/animatable_transform.h"
#include "third_party/blink/renderer/core/css/properties/css_property.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

#include <type_traits>

namespace blink {

void AnimatedStyleBuilder::ApplyProperty(CSSPropertyID property,
                                         ComputedStyle& style,
                                         const AnimatableValue* value) {
#if DCHECK_IS_ON()
  DCHECK(CSSProperty::Get(property).IsInterpolable());
#endif
  switch (property) {
    case CSSPropertyOpacity:
      // Avoiding a value of 1 forces a layer to be created.
      style.SetOpacity(clampTo<float>(ToAnimatableDouble(value)->ToDouble(), 0,
                                      nextafterf(1, 0)));
      return;
    case CSSPropertyTransform: {
      const TransformOperations& operations =
          ToAnimatableTransform(value)->GetTransformOperations();
      // FIXME: This normalization (handling of 'none') should be performed at
      // input in AnimatableValueFactory.
      if (operations.size() == 0) {
        style.SetTransform(TransformOperations(true));
        return;
      }
      double source_zoom = ToAnimatableTransform(value)->Zoom();
      double destination_zoom = style.EffectiveZoom();
      style.SetTransform(source_zoom == destination_zoom
                             ? operations
                             : operations.Zoom(destination_zoom / source_zoom));
      return;
    }
    case CSSPropertyFilter:
      style.SetFilter(ToAnimatableFilterOperations(value)->Operations());
      return;

    default:
      NOTREACHED();
  }
}

}  // namespace blink

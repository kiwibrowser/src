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

#include "third_party/blink/renderer/core/animation/animatable/animatable_filter_operations.h"

#include <algorithm>

namespace blink {

scoped_refptr<AnimatableValue> AnimatableFilterOperations::InterpolateTo(
    const AnimatableValue* value,
    double fraction) const {
  const AnimatableFilterOperations* target =
      ToAnimatableFilterOperations(value);

  if (!Operations().CanInterpolateWith(target->Operations()))
    return DefaultInterpolateTo(this, value, fraction);

  FilterOperations result;
  size_t from_size = Operations().size();
  size_t to_size = target->Operations().size();
  size_t size = std::max(from_size, to_size);
  for (size_t i = 0; i < size; i++) {
    FilterOperation* from =
        (i < from_size) ? operation_wrapper_->Operations().Operations()[i].Get()
                        : nullptr;
    FilterOperation* to =
        (i < to_size)
            ? target->operation_wrapper_->Operations().Operations()[i].Get()
            : nullptr;
    FilterOperation* blended_op = FilterOperation::Blend(from, to, fraction);
    if (blended_op)
      result.Operations().push_back(blended_op);
    else
      NOTREACHED();
  }
  return AnimatableFilterOperations::Create(result);
}

}  // namespace blink

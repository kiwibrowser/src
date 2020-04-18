// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_layout_utils.h"

#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_result.h"

namespace blink {

bool IsBlockLayoutComplete(const NGConstraintSpace& space,
                           const NGLayoutResult& result) {
  if (result.Status() != NGLayoutResult::kSuccess)
    return false;
  if (space.IsIntermediateLayout())
    return false;
  // Check that we're done positioning pending floats.
  return !result.AdjoiningFloatTypes() || result.BfcOffset() ||
         space.FloatsBfcOffset();
}

}  // namespace blink

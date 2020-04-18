// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/inline/ng_line_utils.h"

#include "third_party/blink/renderer/core/editing/position_with_affinity.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_caret_position.h"
#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment.h"

namespace blink {

const NGPaintFragment* NGContainingLineBoxOf(
    const PositionWithAffinity& position) {
  const NGCaretPosition caret_position = ComputeNGCaretPosition(position);
  if (caret_position.IsNull())
    return nullptr;
  return caret_position.fragment->ContainerLineBox();
}

bool InSameNGLineBox(const PositionWithAffinity& position1,
                     const PositionWithAffinity& position2) {
  const NGPaintFragment* line_box1 = NGContainingLineBoxOf(position1);
  if (!line_box1)
    return false;

  const NGPaintFragment* line_box2 = NGContainingLineBoxOf(position2);
  return line_box1 == line_box2;
}

}  // namespace blink

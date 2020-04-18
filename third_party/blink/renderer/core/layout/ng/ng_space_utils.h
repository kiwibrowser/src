// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGSpaceUtils_h
#define NGSpaceUtils_h

#include "base/optional.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/layout_unit.h"

namespace blink {

class ComputedStyle;
struct NGBfcOffset;

// Whether child's constraint space should shrink to its intrinsic width.
// This is needed for buttons, select, input, floats and orthogonal children.
// See LayoutBox::sizesLogicalWidthToFitContent for the rationale behind this.
bool ShouldShrinkToFit(const ComputedStyle& parent_style,
                       const ComputedStyle& style);

// Adjusts {@code offset} to the clearance line.
CORE_EXPORT bool AdjustToClearance(LayoutUnit clearance_offset,
                                   NGBfcOffset* offset);

}  // namespace blink

#endif  // NGSpaceUtils_h

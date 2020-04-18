// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGFloatsUtils_h
#define NGFloatsUtils_h

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/style/computed_style_constants.h"
#include "third_party/blink/renderer/platform/layout_unit.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class NGConstraintSpace;
class NGContainerFragmentBuilder;
class NGExclusionSpace;
struct NGPositionedFloat;
struct NGUnpositionedFloat;

enum NGFloatTypeValue {
  kFloatTypeNone = 0b00,
  kFloatTypeLeft = 0b01,
  kFloatTypeRight = 0b10,
  kFloatTypeBoth = 0b11
};
typedef int NGFloatTypes;

// Returns the inline size (relative to {@code parent_space}) of the
// unpositioned float. If the float is in a different writing mode, this will
// perform a layout.
CORE_EXPORT LayoutUnit
ComputeInlineSizeForUnpositionedFloat(const NGConstraintSpace& parent_space,
                                      NGUnpositionedFloat* unpositioned_float);

// Positions {@code unpositioned_float} into {@code new_parent_space}.
// @returns A positioned float.
CORE_EXPORT NGPositionedFloat
PositionFloat(LayoutUnit origin_block_offset,
              LayoutUnit parent_bfc_block_offset,
              NGUnpositionedFloat*,
              const NGConstraintSpace& parent_space,
              NGExclusionSpace* exclusion_space);

// Positions the list of {@code unpositioned_floats}. Adds them as exclusions to
// {@code space}.
CORE_EXPORT const Vector<NGPositionedFloat> PositionFloats(
    LayoutUnit origin_block_offset,
    LayoutUnit container_block_offset,
    const Vector<scoped_refptr<NGUnpositionedFloat>>& unpositioned_floats,
    const NGConstraintSpace& space,
    NGExclusionSpace* exclusion_space);

// Add a pending float to the list. It will be committed (positioned) once we
// have resolved the BFC offset.
void AddUnpositionedFloat(
    Vector<scoped_refptr<NGUnpositionedFloat>>* unpositioned_floats,
    NGContainerFragmentBuilder* fragment_builder,
    scoped_refptr<NGUnpositionedFloat> unpositioned_float);

NGFloatTypes ToFloatTypes(EClear clear);

}  // namespace blink

#endif  // NGFloatsUtils_h

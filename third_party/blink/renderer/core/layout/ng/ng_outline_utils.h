// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_NG_OUTLINE_UTILS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_NG_OUTLINE_UTILS_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class ComputedStyle;
class LayoutObject;
class LayoutRect;
class NGPhysicalFragment;
class NGPhysicalBoxFragment;
struct NGPhysicalOffset;
struct NGPhysicalOffsetRect;

class CORE_EXPORT NGOutlineUtils {
  STATIC_ONLY(NGOutlineUtils);

 public:
  using FragmentMap = HashMap<const LayoutObject*, const NGPhysicalFragment*>;
  using OutlineRectMap = HashMap<const LayoutObject*, Vector<LayoutRect>>;

  static void CollectDescendantOutlines(const NGPhysicalBoxFragment& container,
                                        const NGPhysicalOffset& paint_offset,
                                        FragmentMap* anchor_fragment_map,
                                        OutlineRectMap* outline_rect_map);

  // Union of all outline rectangles, including outline thickness.
  static NGPhysicalOffsetRect ComputeEnclosingOutline(
      const ComputedStyle& style,
      const Vector<LayoutRect>& outlines);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_NG_OUTLINE_UTILS_H_

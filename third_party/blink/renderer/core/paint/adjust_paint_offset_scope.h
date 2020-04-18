// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_ADJUST_PAINT_OFFSET_SCOPE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_ADJUST_PAINT_OFFSET_SCOPE_H_

#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/graphics/paint/scoped_paint_chunk_properties.h"

namespace blink {

class AdjustPaintOffsetScope {
  STACK_ALLOCATED();

 public:
  AdjustPaintOffsetScope(const LayoutBox& box,
                         const PaintInfo& paint_info,
                         const LayoutPoint& paint_offset)
      : old_paint_info_(paint_info) {
    if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled() ||
        !AdjustPaintOffset(box))
      adjusted_paint_offset_ = paint_offset + box.Location();
  }

  AdjustPaintOffsetScope(const NGPaintFragment& fragment,
                         const PaintInfo& paint_info,
                         const LayoutPoint& paint_offset)
      : old_paint_info_(paint_info) {
    DCHECK(fragment.GetLayoutObject());
    const LayoutBox& box = ToLayoutBox(*fragment.GetLayoutObject());
    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
        AdjustPaintOffset(box))
      return;
    if (UNLIKELY(box.HasSelfPaintingLayer())) {
      //   There is no containing block here, we are painting from origin.
      //   paint_offset is 0,0
      //   box.Location is offset from Layer()
      adjusted_paint_offset_ = paint_offset + box.Location();
    } else {
      adjusted_paint_offset_ = paint_offset + fragment.Offset().ToLayoutPoint();
    }
  }

  const PaintInfo& GetPaintInfo() const {
    return new_paint_info_ ? *new_paint_info_ : old_paint_info_;
  }

  PaintInfo& MutablePaintInfo() {
    if (!new_paint_info_)
      new_paint_info_.emplace(old_paint_info_);
    return *new_paint_info_;
  }

  LayoutPoint AdjustedPaintOffset() const { return adjusted_paint_offset_; }

  // True if child will use LayoutObject::Location to compute adjusted_offset.
  static bool WillUseLegacyLocation(const LayoutBox* child);

 private:
  // Returns true if paint info and offset has been adjusted.
  bool AdjustPaintOffset(const LayoutBox&);

  const PaintInfo& old_paint_info_;
  LayoutPoint adjusted_paint_offset_;
  base::Optional<PaintInfo> new_paint_info_;
  base::Optional<ScopedPaintChunkProperties> contents_properties_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_ADJUST_PAINT_OFFSET_SCOPE_H_

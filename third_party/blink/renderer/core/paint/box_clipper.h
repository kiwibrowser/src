// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BOX_CLIPPER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BOX_CLIPPER_H_

#include "third_party/blink/renderer/core/paint/box_clipper_base.h"
#include "third_party/blink/renderer/platform/geometry/layout_point.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"

namespace blink {

class LayoutBox;

enum ContentsClipBehavior { kForceContentsClip, kSkipContentsClipIfPossible };

class BoxClipper : public BoxClipperBase {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  BoxClipper(const LayoutBox&,
             const PaintInfo&,
             const LayoutPoint& accumulated_offset,
             ContentsClipBehavior);
  ~BoxClipper();

 private:
  const LayoutBox& box_;
  const PaintInfo& paint_info_;
  DisplayItem::Type clip_type_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BOX_CLIPPER_H_

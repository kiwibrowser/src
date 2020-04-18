// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_NG_NG_FRAGMENT_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_NG_NG_FRAGMENT_PAINTER_H_

#include "third_party/blink/renderer/core/paint/object_painter_base.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"

namespace blink {

class LayoutObject;
class LayoutPoint;
class NGPaintFragment;
struct PaintInfo;

// Generic fragment painter for paint logic shared between all types of
// fragments. LayoutNG version of ObjectPainter, based on ObjectPainterBase.
class NGFragmentPainter : public ObjectPainterBase {
  STACK_ALLOCATED();

 public:
  NGFragmentPainter(const NGPaintFragment& paint_fragment)
      : paint_fragment_(paint_fragment) {}

  void PaintOutline(const PaintInfo&, const LayoutPoint& paint_offset);
  void PaintDescendantOutlines(const PaintInfo&,
                               const LayoutPoint& paint_offset);

  void AddPDFURLRectIfNeeded(const PaintInfo&, const LayoutPoint& paint_offset);

 private:
  void CollectDescendantOutlines(
      const LayoutPoint& paint_offset,
      HashMap<const LayoutObject*, NGPaintFragment*>* anchor_fragment_map,
      HashMap<const LayoutObject*, Vector<LayoutRect>>* outline_rect_map);

  const NGPaintFragment& paint_fragment_;
};

}  // namespace blink

#endif

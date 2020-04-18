// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FRAME_SET_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FRAME_SET_PAINTER_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class IntRect;
struct PaintInfo;
class LayoutFrameSet;
class LayoutPoint;

class FrameSetPainter {
  STACK_ALLOCATED();

 public:
  FrameSetPainter(const LayoutFrameSet& layout_frame_set)
      : layout_frame_set_(layout_frame_set) {}

  void Paint(const PaintInfo&, const LayoutPoint& paint_offset);

 private:
  void PaintBorders(const PaintInfo&, const LayoutPoint& adjusted_paint_offset);
  void PaintChildren(const PaintInfo&,
                     const LayoutPoint& adjusted_paint_offset);
  void PaintRowBorder(const PaintInfo&, const IntRect&);
  void PaintColumnBorder(const PaintInfo&, const IntRect&);

  const LayoutFrameSet& layout_frame_set_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FRAME_SET_PAINTER_H_

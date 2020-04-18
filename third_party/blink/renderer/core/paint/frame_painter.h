// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FRAME_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FRAME_PAINTER_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/paint/paint_phase.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class CullRect;
class GraphicsContext;
class IntRect;
class LocalFrameView;
class Scrollbar;

class FramePainter {
  STACK_ALLOCATED();

 public:
  explicit FramePainter(const LocalFrameView& frame_view)
      : frame_view_(&frame_view) {}

  void Paint(GraphicsContext&, const GlobalPaintFlags, const CullRect&);
  void PaintScrollbars(GraphicsContext&, const IntRect&);
  void PaintContents(GraphicsContext&, const GlobalPaintFlags, const IntRect&);
  void PaintScrollCorner(GraphicsContext&, const IntRect& corner_rect);

 private:
  void PaintScrollbar(GraphicsContext&, Scrollbar&, const IntRect&);

  const LocalFrameView& GetFrameView();

  Member<const LocalFrameView> frame_view_;
  static bool in_paint_contents_;

  DISALLOW_COPY_AND_ASSIGN(FramePainter);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FRAME_PAINTER_H_

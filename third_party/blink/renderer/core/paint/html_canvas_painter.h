// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_HTML_CANVAS_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_HTML_CANVAS_PAINTER_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

struct PaintInfo;
class LayoutPoint;
class LayoutHTMLCanvas;

class HTMLCanvasPainter {
  STACK_ALLOCATED();

 public:
  HTMLCanvasPainter(const LayoutHTMLCanvas& layout_html_canvas)
      : layout_html_canvas_(layout_html_canvas) {}
  void PaintReplaced(const PaintInfo&, const LayoutPoint&);

 private:
  const LayoutHTMLCanvas& layout_html_canvas_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_HTML_CANVAS_PAINTER_H_

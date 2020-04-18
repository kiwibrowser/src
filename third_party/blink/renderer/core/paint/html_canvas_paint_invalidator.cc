// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/html_canvas_paint_invalidator.h"

#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/core/layout/layout_html_canvas.h"
#include "third_party/blink/renderer/core/paint/box_paint_invalidator.h"
#include "third_party/blink/renderer/core/paint/paint_invalidator.h"

namespace blink {

PaintInvalidationReason HTMLCanvasPaintInvalidator::InvalidatePaint() {
  auto* element = ToHTMLCanvasElement(html_canvas_.GetNode());
  if (element->IsDirty())
    element->DoDeferredPaintInvalidation();

  return BoxPaintInvalidator(html_canvas_, context_).InvalidatePaint();
}

}  // namespace blink

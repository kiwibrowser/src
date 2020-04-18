// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_HTML_CANVAS_PAINT_INVALIDATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_HTML_CANVAS_PAINT_INVALIDATOR_H_

#include "third_party/blink/renderer/platform/graphics/paint_invalidation_reason.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class LayoutHTMLCanvas;
struct PaintInvalidatorContext;

class HTMLCanvasPaintInvalidator {
  STACK_ALLOCATED();

 public:
  HTMLCanvasPaintInvalidator(const LayoutHTMLCanvas& html_canvas,
                             const PaintInvalidatorContext& context)
      : html_canvas_(html_canvas), context_(context) {}

  PaintInvalidationReason InvalidatePaint();

 private:
  const LayoutHTMLCanvas& html_canvas_;
  const PaintInvalidatorContext& context_;
};

}  // namespace blink

#endif

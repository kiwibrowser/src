// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "third_party/blink/renderer/modules/canvas/htmlcanvas/canvas_context_creation_attributes_helpers.h"

#include "third_party/blink/renderer/core/html/canvas/canvas_context_creation_attributes_core.h"
#include "third_party/blink/renderer/modules/canvas/htmlcanvas/canvas_context_creation_attributes_module.h"
#include "third_party/blink/renderer/modules/xr/xr_device.h"

namespace blink {

CanvasContextCreationAttributesCore ToCanvasContextCreationAttributes(
    const CanvasContextCreationAttributesModule& attrs) {
  CanvasContextCreationAttributesCore result;
  result.alpha = attrs.alpha();
  result.antialias = attrs.antialias();
  result.color_space = attrs.colorSpace();
  result.depth = attrs.depth();
  result.fail_if_major_performance_caveat =
      attrs.failIfMajorPerformanceCaveat();
  result.low_latency = attrs.lowLatency();
  result.pixel_format = attrs.pixelFormat();
  result.premultiplied_alpha = attrs.premultipliedAlpha();
  result.preserve_drawing_buffer = attrs.preserveDrawingBuffer();
  result.stencil = attrs.stencil();
  result.compatible_xr_device = attrs.compatibleXRDevice();
  return result;
}

}  // namespace blink

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/compositing_recorder.h"

#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/compositing_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"

namespace blink {

CompositingRecorder::CompositingRecorder(GraphicsContext& graphics_context,
                                         const DisplayItemClient& client,
                                         const SkBlendMode xfer_mode,
                                         const float opacity,
                                         const FloatRect* bounds,
                                         ColorFilter color_filter)
    : client_(client), graphics_context_(graphics_context) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;
  graphics_context.GetPaintController()
      .CreateAndAppend<BeginCompositingDisplayItem>(client_, xfer_mode, opacity,
                                                    bounds, color_filter);
}

CompositingRecorder::~CompositingRecorder() {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;
  graphics_context_.GetPaintController().EndItem<EndCompositingDisplayItem>(
      client_);
}

}  // namespace blink

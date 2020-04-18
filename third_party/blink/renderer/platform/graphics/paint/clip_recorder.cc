// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/clip_recorder.h"

#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"

namespace blink {

ClipRecorder::ClipRecorder(GraphicsContext& context,
                           const DisplayItemClient& client,
                           DisplayItem::Type type,
                           const IntRect& clip_rect)
    : client_(client), context_(context), type_(type) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;
  context_.GetPaintController().CreateAndAppend<ClipDisplayItem>(client_, type,
                                                                 clip_rect);
}

ClipRecorder::~ClipRecorder() {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;
  context_.GetPaintController().EndItem<EndClipDisplayItem>(
      client_, DisplayItem::ClipTypeToEndClipType(type_));
}

}  // namespace blink

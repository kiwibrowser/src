// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/clip_path_recorder.h"

#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_path_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"

namespace blink {

ClipPathRecorder::ClipPathRecorder(GraphicsContext& context,
                                   const DisplayItemClient& client,
                                   const Path& clip_path)
    : context_(context), client_(client) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;
  context_.GetPaintController().CreateAndAppend<BeginClipPathDisplayItem>(
      client_, clip_path);
}

ClipPathRecorder::~ClipPathRecorder() {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;
  context_.GetPaintController().EndItem<EndClipPathDisplayItem>(client_);
}

}  // namespace blink

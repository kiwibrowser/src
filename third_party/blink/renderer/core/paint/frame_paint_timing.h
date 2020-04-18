// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FRAME_PAINT_TIMING_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FRAME_PAINT_TIMING_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/paint/paint_timing.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class FramePaintTiming {
  STACK_ALLOCATED();

 public:
  FramePaintTiming(GraphicsContext& context, const LocalFrame* frame)
      : context_(context), frame_(frame) {
    context_.GetPaintController().BeginFrame(frame_);
  }

  ~FramePaintTiming() {
    DCHECK(frame_->GetDocument());
    FrameFirstPaint result = context_.GetPaintController().EndFrame(frame_);
    PaintTiming::From(*frame_->GetDocument())
        .NotifyPaint(result.first_painted, result.text_painted,
                     result.image_painted);
  }

 private:
  GraphicsContext& context_;
  Member<const LocalFrame> frame_;
  DISALLOW_COPY_AND_ASSIGN(FramePaintTiming);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FRAME_PAINT_TIMING_H_

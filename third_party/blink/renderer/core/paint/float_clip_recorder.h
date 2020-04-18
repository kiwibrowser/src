// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FLOAT_CLIP_RECORDER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FLOAT_CLIP_RECORDER_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/paint/paint_phase.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class FloatClipRecorder {
  USING_FAST_MALLOC(FloatClipRecorder);

 public:
  FloatClipRecorder(GraphicsContext&,
                    const DisplayItemClient&,
                    PaintPhase,
                    const FloatRect&);
  FloatClipRecorder(GraphicsContext&,
                    const DisplayItemClient&,
                    DisplayItem::Type,
                    const FloatRect&);

  ~FloatClipRecorder();

 private:
  GraphicsContext& context_;
  const DisplayItemClient& client_;
  DisplayItem::Type clip_type_;
  DISALLOW_COPY_AND_ASSIGN(FloatClipRecorder);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FLOAT_CLIP_RECORDER_H_

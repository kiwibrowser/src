// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_SCROLL_RECORDER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_SCROLL_RECORDER_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/paint/paint_phase.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class GraphicsContext;

// Emits display items which represent a region which is scrollable, so that it
// can be translated by the scroll offset.
class CORE_EXPORT ScrollRecorder {
  USING_FAST_MALLOC(ScrollRecorder);

 public:
  ScrollRecorder(GraphicsContext&,
                 const DisplayItemClient&,
                 DisplayItem::Type,
                 const IntSize& current_offset);
  ScrollRecorder(GraphicsContext&,
                 const DisplayItemClient&,
                 PaintPhase,
                 const IntSize& current_offset);
  ~ScrollRecorder();

 private:
  const DisplayItemClient& client_;
  DisplayItem::Type begin_item_type_;
  GraphicsContext& context_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_SCROLL_RECORDER_H_

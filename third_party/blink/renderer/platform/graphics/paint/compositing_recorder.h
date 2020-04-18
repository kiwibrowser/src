// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_COMPOSITING_RECORDER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_COMPOSITING_RECORDER_H_

#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/graphics/graphics_types.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/skia/include/core/SkBlendMode.h"

namespace blink {

class GraphicsContext;

class PLATFORM_EXPORT CompositingRecorder {
  USING_FAST_MALLOC(CompositingRecorder);

 public:
  // If bounds is provided, the content will be explicitly clipped to those
  // bounds.
  CompositingRecorder(GraphicsContext&,
                      const DisplayItemClient&,
                      const SkBlendMode,
                      const float opacity,
                      const FloatRect* bounds = nullptr,
                      ColorFilter = kColorFilterNone);

  ~CompositingRecorder();

 private:
  const DisplayItemClient& client_;
  GraphicsContext& graphics_context_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_COMPOSITING_RECORDER_H_

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_CANVAS_METRICS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_CANVAS_METRICS_H_

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class PLATFORM_EXPORT CanvasMetrics {
  STATIC_ONLY(CanvasMetrics);

 public:
  enum CanvasContextUsage {
    kCanvasCreated = 0,
    kGPUAccelerated2DCanvasImageBufferCreated = 1,
    kUnaccelerated2DCanvasImageBufferCreated = 3,
    kAccelerated2DCanvasGPUContextLost = 4,
    kUnaccelerated2DCanvasImageBufferCreationFailed = 5,
    kGPUAccelerated2DCanvasImageBufferCreationFailed = 6,
    kGPUAccelerated2DCanvasDeferralDisabled = 8,
    kGPUAccelerated2DCanvasSurfaceCreationFailed = 9,
    kNumberOfUsages
  };

  static void CountCanvasContextUsage(const CanvasContextUsage);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_CANVAS_METRICS_H_

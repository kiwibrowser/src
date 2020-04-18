// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_TRANSFORM_RECORDER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_TRANSFORM_RECORDER_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class GraphicsContext;
class AffineTransform;

class CORE_EXPORT TransformRecorder {
  STACK_ALLOCATED();

 public:
  TransformRecorder(GraphicsContext&,
                    const DisplayItemClient&,
                    const AffineTransform&);
  ~TransformRecorder();

 private:
  GraphicsContext& context_;
  const DisplayItemClient& client_;
  bool skip_recording_for_identity_transform_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_TRANSFORM_RECORDER_H_

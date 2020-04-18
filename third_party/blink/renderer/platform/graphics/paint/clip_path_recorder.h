// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_CLIP_PATH_RECORDER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_CLIP_PATH_RECORDER_H_

#include "third_party/blink/renderer/platform/graphics/paint/display_item_client.h"
#include "third_party/blink/renderer/platform/graphics/path.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class GraphicsContext;

class PLATFORM_EXPORT ClipPathRecorder {
  USING_FAST_MALLOC(ClipPathRecorder);
  WTF_MAKE_NONCOPYABLE(ClipPathRecorder);

 public:
  ClipPathRecorder(GraphicsContext&, const DisplayItemClient&, const Path&);
  ~ClipPathRecorder();

 private:
  GraphicsContext& context_;
  const DisplayItemClient& client_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_CLIP_PATH_RECORDER_H_
